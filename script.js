const firebaseConfig = {
    apiKey: "AIzaSyC7XlN2DM38tq19LJuT8WyGhcOVcRxfZec",
    authDomain: "diabetic-care-tracker.firebaseapp.com",
    databaseURL: "https://diabetic-care-tracker-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "diabetic-care-tracker",
    storageBucket: "diabetic-care-tracker.firebasestorage.app",
    messagingSenderId: "525282491981",
    appId: "1:525282491981:web:7dd6e3a6010ae317e9a604"
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);
const auth = firebase.auth();
const database = firebase.database();

let currentUser = null;

// ALERT SOUND
function playAlertSound() {
    const audioContext = new (window.AudioContext || window.webkitAudioContext)();
    const oscillator = audioContext.createOscillator();
    const gainNode = audioContext.createGain();

    oscillator.connect(gainNode);
    gainNode.connect(audioContext.destination);

    oscillator.frequency.value = 800;
    oscillator.type = 'sine';

    gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
    gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);

    oscillator.start(audioContext.currentTime);
    oscillator.stop(audioContext.currentTime + 0.5);
}

// ===========================
// ALERT FUNCTIONS
// ===========================
function showAlert(elementId, message, type = 'success') {
    const alertElement = document.getElementById(elementId);
    alertElement.className = `alert alert-${type} show`;
    alertElement.textContent = message;
    
    if (type === 'danger' || type === 'warning') {
        playAlertSound();
    }

    setTimeout(() => {
        alertElement.classList.remove('show');
    }, 5000);
}

// ===========================
// AUTHENTICATION
// ===========================
document.getElementById('showRegister').addEventListener('click', () => {
    document.getElementById('loginScreen').classList.add('hidden');
    document.getElementById('registerScreen').classList.remove('hidden');
});

document.getElementById('showLogin').addEventListener('click', () => {
    document.getElementById('registerScreen').classList.add('hidden');
    document.getElementById('loginScreen').classList.remove('hidden');
});

document.getElementById('registerForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    const name = document.getElementById('registerName').value;
    const email = document.getElementById('registerEmail').value;
    const password = document.getElementById('registerPassword').value;

    try {
        const userCredential = await auth.createUserWithEmailAndPassword(email, password);
        await database.ref('users/' + userCredential.user.uid).set({
            name: name,
            email: email,
            createdAt: new Date().toISOString()
        });
        showAlert('registerAlert', 'Account created successfully!', 'success');
        setTimeout(() => {
            document.getElementById('registerScreen').classList.add('hidden');
            document.getElementById('loginScreen').classList.remove('hidden');
        }, 2000);
    } catch (error) {
        showAlert('registerAlert', error.message, 'danger');
    }
});

document.getElementById('loginForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    const email = document.getElementById('loginEmail').value;
    const password = document.getElementById('loginPassword').value;

    try {
        await auth.signInWithEmailAndPassword(email, password);
    } catch (error) {
        showAlert('loginAlert', error.message, 'danger');
    }
});

document.getElementById('logoutBtn').addEventListener('click', () => {
    auth.signOut();
});

// Monitor auth state
auth.onAuthStateChanged(async (user) => {
    if (user) {
        currentUser = user;
        document.getElementById('loginScreen').classList.add('hidden');
        document.getElementById('registerScreen').classList.add('hidden');
        document.getElementById('dashboard').classList.add('active');

        const userSnapshot = await database.ref('users/' + user.uid).once('value');
        const userData = userSnapshot.val();
        document.getElementById('userName').textContent = `Welcome back, ${userData.name}`;

        loadDashboardData();
        checkReminders();
        setInterval(checkReminders, 60000); // Check every minute
    } else {
        currentUser = null;
        document.getElementById('dashboard').classList.remove('active');
        document.getElementById('loginScreen').classList.remove('hidden');
    }
});

// ===========================
// BLOOD SUGAR TRACKING
// ===========================
async function addBloodSugarReading() {
    if (!currentUser) return;

    const value = document.getElementById('bloodSugarValue').value;
    const type = document.getElementById('readingType').value;

    if (!value) {
        showAlert('dashboardAlert', 'Please enter blood sugar value', 'warning');
        return;
    }

    const reading = {
        value: parseFloat(value),
        type: type,
        timestamp: new Date().toISOString(),
        date: new Date().toLocaleDateString(),
        time: new Date().toLocaleTimeString()
    };

    try {
        await database.ref(`bloodSugar/${currentUser.uid}`).push(reading);
        document.getElementById('bloodSugarValue').value = '';
        
        // Alert for abnormal readings
        if (value < 70) {
            showAlert('dashboardAlert', '⚠️ Low blood sugar detected! Please take necessary action.', 'danger');
            // Send urgent notification to ESP8266
            sendNotificationToESP('low_glucose', `LOW GLUCOSE ALERT: ${value} mg/dL - Take action immediately!`);
        } else if (value > 180) {
            showAlert('dashboardAlert', '⚠️ High blood sugar detected! Please monitor closely.', 'warning');
            // Send notification to ESP8266
            sendNotificationToESP('high_glucose', `HIGH GLUCOSE: ${value} mg/dL - Monitor closely`);
        } else {
            showAlert('dashboardAlert', '✓ Blood sugar reading added successfully', 'success');
        }

        loadBloodSugarReadings();
        updateStats();
    } catch (error) {
        showAlert('dashboardAlert', 'Error adding reading: ' + error.message, 'danger');
    }
}

async function loadBloodSugarReadings() {
    if (!currentUser) return;

    const snapshot = await database.ref(`bloodSugar/${currentUser.uid}`).orderByChild('timestamp').limitToLast(10).once('value');
    const readings = [];
    snapshot.forEach((child) => {
        readings.push({ id: child.key, ...child.val() });
    });

    const listElement = document.getElementById('readingsList');
    if (readings.length === 0) {
        listElement.innerHTML = `
            <div class="empty-state">
                <svg width="48" height="48" viewBox="0 0 48 48" fill="none">
                    <circle cx="24" cy="24" r="20" stroke="currentColor" stroke-width="2" opacity="0.2"/>
                    <path d="M24 16v16M16 24h16" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                </svg>
                <p>No readings recorded yet</p>
            </div>
        `;
        return;
    }

    listElement.innerHTML = readings.reverse().map(reading => {
        let className = 'reading-item';
        if (reading.value < 70) className += ' low';
        if (reading.value > 180) className += ' high';

        return `
            <div class="${className}">
                <div class="info">
                    <div class="value-large">${reading.value} <span style="font-size: 1rem; font-weight: 500;">mg/dL</span></div>
                    <div class="details">${reading.type.replace('-', ' ')} • ${reading.date} ${reading.time}</div>
                </div>
                <button class="delete-btn" onclick="deleteReading('${reading.id}')">Delete</button>
            </div>
        `;
    }).join('');
}

async function deleteReading(id) {
    if (!currentUser || !confirm('Delete this reading?')) return;
    
    try {
        await database.ref(`bloodSugar/${currentUser.uid}/${id}`).remove();
        loadBloodSugarReadings();
        updateStats();
        showAlert('dashboardAlert', '✓ Reading deleted', 'success');
    } catch (error) {
        showAlert('dashboardAlert', 'Error deleting reading', 'danger');
    }
}

// ===========================
// MEDICATION TRACKING
// ===========================
async function addMedication() {
    if (!currentUser) return;

    const name = document.getElementById('medName').value;
    const dosage = document.getElementById('medDosage').value;
    const time = document.getElementById('medTime').value;
    const frequency = document.getElementById('medFrequency').value;

    if (!name || !dosage || !time) {
        showAlert('dashboardAlert', 'Please fill all medication fields', 'warning');
        return;
    }

    const medication = {
        name: name,
        dosage: dosage,
        time: time,
        frequency: frequency,
        createdAt: new Date().toISOString(),
        active: true
    };

    try {
        await database.ref(`medications/${currentUser.uid}`).push(medication);
        document.getElementById('medName').value = '';
        document.getElementById('medDosage').value = '';
        document.getElementById('medTime').value = '';
        showAlert('dashboardAlert', '✓ Medication added successfully', 'success');
        loadMedications();
    } catch (error) {
        showAlert('dashboardAlert', 'Error adding medication', 'danger');
    }
}

async function loadMedications() {
    if (!currentUser) return;

    const snapshot = await database.ref(`medications/${currentUser.uid}`).once('value');
    const medications = [];
    snapshot.forEach((child) => {
        medications.push({ id: child.key, ...child.val() });
    });

    const listElement = document.getElementById('medicationList');
    if (medications.length === 0) {
        listElement.innerHTML = `
            <div class="empty-state">
                <svg width="48" height="48" viewBox="0 0 48 48" fill="none">
                    <circle cx="24" cy="24" r="20" stroke="currentColor" stroke-width="2" opacity="0.2"/>
                    <path d="M24 16v16M16 24h16" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                </svg>
                <p>No medications scheduled</p>
            </div>
        `;
        return;
    }

    // Get today's intake records
    const today = new Date().toLocaleDateString();
    const intakeSnapshot = await database.ref(`medicationIntake/${currentUser.uid}`).orderByChild('date').equalTo(today).once('value');
    const takenToday = new Set();
    intakeSnapshot.forEach((child) => {
        takenToday.add(child.val().medicationId);
    });

    listElement.innerHTML = medications.map(med => {
        const taken = takenToday.has(med.id);
        return `
            <div class="medication-item ${taken ? 'taken' : ''}">
                <div class="med-header">
                    <div class="med-name">${med.name}</div>
                </div>
                <div class="med-details">💊 Dosage: ${med.dosage}</div>
                <div class="med-details">⏰ Time: ${med.time}</div>
                <div class="med-details">📅 Frequency: ${med.frequency.replace('-', ' ')}</div>
                <div class="actions">
                    ${!taken ? `<button class="btn-take" onclick="markAsTaken('${med.id}', '${med.name}', '${med.dosage}')">✓ Mark as Taken</button>` : '<span style="color: var(--color-success); font-weight: 600;">✓ Taken Today</span>'}
                    <button class="btn-delete" onclick="deleteMedication('${med.id}')">Delete</button>
                </div>
            </div>
        `;
    }).join('');

    updateStats();
}

async function markAsTaken(medicationId, name, dosage) {
    if (!currentUser) return;

    const intake = {
        medicationId: medicationId,
        medicationName: name,
        dosage: dosage,
        timestamp: new Date().toISOString(),
        date: new Date().toLocaleDateString(),
        time: new Date().toLocaleTimeString()
    };

    try {
        await database.ref(`medicationIntake/${currentUser.uid}`).push(intake);
        showAlert('dashboardAlert', `✓ Marked ${name} as taken`, 'success');
        loadMedications();
        loadMedicationHistory();
    } catch (error) {
        showAlert('dashboardAlert', 'Error recording intake', 'danger');
    }
}

async function deleteMedication(id) {
    if (!currentUser || !confirm('Delete this medication?')) return;

    try {
        await database.ref(`medications/${currentUser.uid}/${id}`).remove();
        loadMedications();
        showAlert('dashboardAlert', '✓ Medication deleted', 'success');
    } catch (error) {
        showAlert('dashboardAlert', 'Error deleting medication', 'danger');
    }
}

async function loadMedicationHistory() {
    if (!currentUser) return;

    const sevenDaysAgo = new Date();
    sevenDaysAgo.setDate(sevenDaysAgo.getDate() - 7);

    const snapshot = await database.ref(`medicationIntake/${currentUser.uid}`).once('value');
    const history = [];
    snapshot.forEach((child) => {
        const intake = child.val();
        if (new Date(intake.timestamp) >= sevenDaysAgo) {
            history.push(intake);
        }
    });

    const tbody = document.getElementById('historyBody');
    if (history.length === 0) {
        tbody.innerHTML = '<tr><td colspan="4"><div class="empty-state-inline">No history available</div></td></tr>';
        return;
    }

    tbody.innerHTML = history.reverse().map(record => `
        <tr>
            <td>${record.date} ${record.time}</td>
            <td><strong>${record.medicationName}</strong></td>
            <td>${record.dosage}</td>
            <td><span style="color: var(--color-success); font-weight: 600;">✓ Taken</span></td>
        </tr>
    `).join('');
}

// ===========================
// REMINDERS
// ===========================
async function addReminder() {
    if (!currentUser) return;

    const time = document.getElementById('reminderTime').value;
    const description = document.getElementById('reminderDescription').value;

    if (!time || !description) {
        showAlert('dashboardAlert', 'Please fill all reminder fields', 'warning');
        return;
    }

    const reminder = {
        time: time,
        description: description,
        active: true,
        createdAt: new Date().toISOString()
    };

    try {
        await database.ref(`reminders/${currentUser.uid}`).push(reminder);
        document.getElementById('reminderTime').value = '';
        document.getElementById('reminderDescription').value = '';
        showAlert('dashboardAlert', '✓ Reminder added successfully', 'success');
        loadReminders();
    } catch (error) {
        showAlert('dashboardAlert', 'Error adding reminder', 'danger');
    }
}

async function loadReminders() {
    if (!currentUser) return;

    const snapshot = await database.ref(`reminders/${currentUser.uid}`).once('value');
    const reminders = [];
    snapshot.forEach((child) => {
        reminders.push({ id: child.key, ...child.val() });
    });

    const listElement = document.getElementById('scheduleList');
    if (reminders.length === 0) {
        listElement.innerHTML = `
            <div class="empty-state">
                <svg width="48" height="48" viewBox="0 0 48 48" fill="none">
                    <circle cx="24" cy="24" r="20" stroke="currentColor" stroke-width="2" opacity="0.2"/>
                    <path d="M24 16v16M16 24h16" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                </svg>
                <p>No reminders set</p>
            </div>
        `;
        return;
    }

    listElement.innerHTML = reminders.sort((a, b) => a.time.localeCompare(b.time)).map(reminder => `
        <div class="schedule-item">
            <div>
                <div class="time">⏰ ${reminder.time}</div>
                <div class="description">${reminder.description}</div>
            </div>
            <button class="btn-delete" onclick="deleteReminder('${reminder.id}')">Delete</button>
        </div>
    `).join('');
}

async function deleteReminder(id) {
    if (!currentUser || !confirm('Delete this reminder?')) return;

    try {
        await database.ref(`reminders/${currentUser.uid}/${id}`).remove();
        loadReminders();
        showAlert('dashboardAlert', '✓ Reminder deleted', 'success');
    } catch (error) {
        showAlert('dashboardAlert', 'Error deleting reminder', 'danger');
    }
}

async function checkReminders() {
    if (!currentUser) return;

    const now = new Date();
    const currentTime = now.toTimeString().slice(0, 5); // HH:MM format

    const snapshot = await database.ref(`reminders/${currentUser.uid}`).once('value');
    snapshot.forEach((child) => {
        const reminder = child.val();
        if (reminder.time === currentTime) {
            playAlertSound();
            showAlert('dashboardAlert', `⏰ Reminder: ${reminder.description}`, 'warning');
            // Send notification to ESP8266
            sendNotificationToESP('reminder', `Reminder: ${reminder.description}`);
        }
    });

    // Check medication times
    const medSnapshot = await database.ref(`medications/${currentUser.uid}`).once('value');
    medSnapshot.forEach((child) => {
        const med = child.val();
        if (med.time === currentTime) {
            playAlertSound();
            showAlert('dashboardAlert', `💊 Time to take: ${med.name} (${med.dosage})`, 'warning');
            // Send notification to ESP8266
            sendNotificationToESP('medication', `Time to take: ${med.name} (${med.dosage})`);
        }
    });
}

// ===========================
// ESP8266 NOTIFICATION SYSTEM
// ===========================
async function sendNotificationToESP(type, message) {
    if (!currentUser) return;
    
    const notificationId = database.ref().push().key;
    const notification = {
        id: notificationId,
        type: type, // medication, reminder, high_glucose, low_glucose
        message: message,
        timestamp: new Date().toISOString(),
        sent: true
    };
    
    try {
        // Send to latest notification path (ESP8266 monitors this)
        await database.ref(`notifications/${currentUser.uid}/latest`).set(notification);
        
        // Also store in history
        await database.ref(`notifications/${currentUser.uid}/history/${notificationId}`).set(notification);
        
        console.log('Notification sent to ESP8266:', type, message);
    } catch (error) {
        console.error('Error sending notification to ESP8266:', error);
    }
}

// ===========================
// STATISTICS
// ===========================
async function updateStats() {
    if (!currentUser) return;

    // Average blood sugar
    const bsSnapshot = await database.ref(`bloodSugar/${currentUser.uid}`).limitToLast(30).once('value');
    let total = 0;
    let count = 0;
    let lastValue = '--';

    bsSnapshot.forEach((child) => {
        const reading = child.val();
        total += reading.value;
        count++;
        lastValue = reading.value;
    });

    if (count > 0) {
        document.getElementById('avgBloodSugar').textContent = Math.round(total / count);
        document.getElementById('lastReading').textContent = lastValue;
    }

    // Medications taken today
    const today = new Date().toLocaleDateString();
    const intakeSnapshot = await database.ref(`medicationIntake/${currentUser.uid}`).orderByChild('date').equalTo(today).once('value');
    const takenCount = intakeSnapshot.numChildren();

    const medSnapshot = await database.ref(`medications/${currentUser.uid}`).once('value');
    const totalMeds = medSnapshot.numChildren();

    document.getElementById('medsTaken').textContent = `${takenCount}/${totalMeds}`;
}

// ===========================
// LOAD ALL DATA
// ===========================
async function loadDashboardData() {
    loadBloodSugarReadings();
    loadMedications();
    loadReminders();
    loadMedicationHistory();
    updateStats();
}

// ===========================
// CALENDAR MODAL
// ===========================
let currentCalendarDate = new Date();
let medicationCalendarData = {};

function openCalendarModal() {
    document.getElementById('calendarModal').classList.add('active');
    currentCalendarDate = new Date();
    loadCalendarData();
}

function closeCalendarModal() {
    document.getElementById('calendarModal').classList.remove('active');
    document.getElementById('selectedDateDetails').classList.add('hidden');
}

function changeMonth(direction) {
    currentCalendarDate.setMonth(currentCalendarDate.getMonth() + direction);
    renderCalendar();
}

async function loadCalendarData() {
    if (!currentUser) return;
    
    // Get all medication intake data
    const snapshot = await database.ref(`medicationIntake/${currentUser.uid}`).once('value');
    medicationCalendarData = {};
    
    snapshot.forEach((child) => {
        const intake = child.val();
        const dateKey = intake.date;
        
        if (!medicationCalendarData[dateKey]) {
            medicationCalendarData[dateKey] = [];
        }
        
        medicationCalendarData[dateKey].push(intake);
    });
    
    renderCalendar();
}

function renderCalendar() {
    const year = currentCalendarDate.getFullYear();
    const month = currentCalendarDate.getMonth();
    
    // Update month/year display
    const monthNames = ['January', 'February', 'March', 'April', 'May', 'June',
                       'July', 'August', 'September', 'October', 'November', 'December'];
    document.getElementById('calendarMonthYear').textContent = `${monthNames[month]} ${year}`;
    
    // Get first day of month and number of days
    const firstDay = new Date(year, month, 1).getDay();
    const daysInMonth = new Date(year, month + 1, 0).getDate();
    const daysInPrevMonth = new Date(year, month, 0).getDate();
    
    const calendarGrid = document.getElementById('calendarGrid');
    calendarGrid.innerHTML = '';
    
    // Add day headers
    const dayHeaders = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
    dayHeaders.forEach(day => {
        const header = document.createElement('div');
        header.className = 'calendar-day-header';
        header.textContent = day;
        calendarGrid.appendChild(header);
    });
    
    // Add previous month's days
    for (let i = firstDay - 1; i >= 0; i--) {
        const day = daysInPrevMonth - i;
        const dayElement = createCalendarDay(day, month - 1, year, true);
        calendarGrid.appendChild(dayElement);
    }
    
    // Add current month's days
    const today = new Date();
    for (let day = 1; day <= daysInMonth; day++) {
        const isToday = today.getDate() === day && 
                       today.getMonth() === month && 
                       today.getFullYear() === year;
        const dayElement = createCalendarDay(day, month, year, false, isToday);
        calendarGrid.appendChild(dayElement);
    }
    
    // Add next month's days to fill grid
    const totalCells = calendarGrid.children.length - 7; // Subtract headers
    const remainingCells = 42 - totalCells; // 6 rows * 7 days
    for (let day = 1; day <= remainingCells; day++) {
        const dayElement = createCalendarDay(day, month + 1, year, true);
        calendarGrid.appendChild(dayElement);
    }
}

function createCalendarDay(day, month, year, isOtherMonth, isToday = false) {
    const dayElement = document.createElement('div');
    dayElement.className = 'calendar-day';
    
    if (isOtherMonth) {
        dayElement.classList.add('other-month');
    }
    
    if (isToday) {
        dayElement.classList.add('today');
    }
    
    // Create the date key for this day
    const date = new Date(year, month, day);
    const dateKey = date.toLocaleDateString();
    
    // Check if there are medications taken on this day
    if (medicationCalendarData[dateKey] && medicationCalendarData[dateKey].length > 0) {
        dayElement.classList.add('has-medication');
    }
    
    const dayNumber = document.createElement('div');
    dayNumber.className = 'calendar-day-number';
    dayNumber.textContent = day;
    dayElement.appendChild(dayNumber);
    
    // Add indicator dot if has medications
    if (medicationCalendarData[dateKey] && medicationCalendarData[dateKey].length > 0) {
        const indicator = document.createElement('div');
        indicator.className = 'calendar-indicator';
        dayElement.appendChild(indicator);
    }
    
    // Add click handler
    dayElement.addEventListener('click', () => {
        showDateDetails(date, dateKey);
    });
    
    return dayElement;
}

function showDateDetails(date, dateKey) {
    // Remove previous selection
    document.querySelectorAll('.calendar-day.selected').forEach(el => {
        el.classList.remove('selected');
    });
    
    // Add selection to clicked day
    event.target.closest('.calendar-day').classList.add('selected');
    
    const detailsContainer = document.getElementById('selectedDateDetails');
    const titleElement = document.getElementById('selectedDateTitle');
    const listElement = document.getElementById('medicationsTakenList');
    
    // Format date nicely
    const options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
    titleElement.textContent = date.toLocaleDateString('en-US', options);
    
    // Get medications for this date
    const medications = medicationCalendarData[dateKey] || [];
    
    if (medications.length === 0) {
        listElement.innerHTML = `
            <div class="no-medications">
                <svg width="48" height="48" viewBox="0 0 48 48" fill="none">
                    <circle cx="24" cy="24" r="20" stroke="currentColor" stroke-width="2" opacity="0.2"/>
                    <path d="M24 16v8M24 32v.01" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                </svg>
                <p>No medications taken on this date</p>
            </div>
        `;
    } else {
        listElement.innerHTML = medications.map(med => `
            <div class="medication-taken-item">
                <div class="medication-taken-info">
                    <div class="medication-taken-name">${med.medicationName}</div>
                    <div class="medication-taken-details">
                        ${med.dosage} • Taken at ${med.time}
                    </div>
                </div>
                <div class="medication-taken-status">
                    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
                        <circle cx="10" cy="10" r="9" stroke="currentColor" stroke-width="2"/>
                        <path d="M6 10l2.5 2.5L14 7" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                    </svg>
                    Taken
                </div>
            </div>
        `).join('');
    }
    
    detailsContainer.classList.remove('hidden');
}

// Close modal when clicking outside
document.addEventListener('click', (e) => {
    const modal = document.getElementById('calendarModal');
    if (e.target === modal) {
        closeCalendarModal();
    }
});

// Close modal with Escape key
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        const modal = document.getElementById('calendarModal');
        if (modal.classList.contains('active')) {
            closeCalendarModal();
        }
    }
});

