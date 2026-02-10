# 🩺 Diabetic Care Tracker

A comprehensive web application for diabetic patients to manage their health, track blood sugar levels, schedule medications, and set reminders.

## Features

### 🔐 User Authentication
- Secure login and registration system
- Firebase Authentication integration
- User-specific data management

### 📊 Blood Sugar Tracking
- Record blood sugar readings with timestamp
- Categorize readings (Fasting, Before Meal, After Meal, Bedtime)
- Visual indicators for high/low readings
- Alert sounds for abnormal readings (< 70 mg/dL or > 180 mg/dL)
- Calculate average blood sugar levels

### 💊 Medication Management
- Add medications with dosage and schedule
- Mark medications as taken
- Track medication history
- Set medication frequencies (Daily, Twice Daily, Three Times Daily, Weekly)
- View 7-day medication history

### ⏰ Reminders & Alerts
- Set custom reminders with time and description
- Automatic medication time alerts
- Audio notifications for reminders and abnormal readings
- Real-time reminder checking every minute

### 📈 Statistics Dashboard
- Average blood sugar over last 30 readings
- Last recorded blood sugar value
- Daily medication adherence tracker
- Visual statistics cards

## 🚀 Setup Instructions

### Prerequisites
- A modern web browser (Chrome, Firefox, Safari, or Edge)
- A Firebase account (free tier works perfectly)

### Firebase Setup

1. **Create a Firebase Project**
   - Go to [Firebase Console](https://console.firebase.google.com/)
   - Click "Add project"
   - Enter a project name (e.g., "Diabetic-Care-Tracker")
   - Disable Google Analytics (optional)
   - Click "Create project"

2. **Enable Authentication**
   - In your Firebase project, go to "Authentication"
   - Click "Get started"
   - Under "Sign-in method" tab, enable "Email/Password"
   - Click "Save"

3. **Setup Realtime Database**
   - Go to "Realtime Database" in the left menu
   - Click "Create Database"
   - Choose location closest to your users
   - Start in **Test mode** (for development)
   - Click "Enable"

4. **Configure Database Rules** (Important for Security)
   - In Realtime Database, go to "Rules" tab
   - Replace the rules with:
   ```json
   {
     "rules": {
       "users": {
         "$uid": {
           ".read": "$uid === auth.uid",
           ".write": "$uid === auth.uid"
         }
       },
       "bloodSugar": {
         "$uid": {
           ".read": "$uid === auth.uid",
           ".write": "$uid === auth.uid"
         }
       },
       "medications": {
         "$uid": {
           ".read": "$uid === auth.uid",
           ".write": "$uid === auth.uid"
         }
       },
       "medicationIntake": {
         "$uid": {
           ".read": "$uid === auth.uid",
           ".write": "$uid === auth.uid"
         }
       },
       "reminders": {
         "$uid": {
           ".read": "$uid === auth.uid",
           ".write": "$uid === auth.uid"
         }
       }
     }
   }
   ```
   - Click "Publish"

5. **Get Firebase Configuration**
   - Go to Project Settings (gear icon near "Project Overview")
   - Scroll down to "Your apps"
   - Click the web icon (`</>`) to add a web app
   - Register your app with a nickname
   - Copy the Firebase configuration object

6. **Configure the Application**
   - Open `diabetic-tracker.html` in a text editor
   - Find the `firebaseConfig` object (around line 460)
   - Replace the placeholder values with your Firebase config:
   ```javascript
   const firebaseConfig = {
       apiKey: "YOUR_API_KEY",
       authDomain: "YOUR_AUTH_DOMAIN",
       databaseURL: "YOUR_DATABASE_URL",
       projectId: "YOUR_PROJECT_ID",
       storageBucket: "YOUR_STORAGE_BUCKET",
       messagingSenderId: "YOUR_MESSAGING_SENDER_ID",
       appId: "YOUR_APP_ID"
   };
   ```

7. **Launch the Application**
   - Open `diabetic-tracker.html` in your web browser
   - Create an account or login

## 📖 How to Use

### First Time Setup
1. Open the application in your browser
2. Click "Create Account"
3. Enter your full name, email, and password
4. Click "Register"
5. Login with your credentials

### Adding Blood Sugar Readings
1. Enter your blood sugar value in mg/dL
2. Select the reading type (Fasting, Before Meal, After Meal, Bedtime)
3. Click "Add"
4. The system will alert you if the reading is too high (>180) or too low (<70)

### Managing Medications
1. Go to the "Medications" card
2. Enter medication name, dosage, time, and frequency
3. Click "Add Medication"
4. When it's time to take your medication:
   - Click "Mark as Taken" to record it
   - The system will alert you at the scheduled time

### Setting Reminders
1. Go to the "Reminders & Schedule" card
2. Enter the time and description
3. Click "Add Reminder"
4. You'll receive an alert with sound at the scheduled time

### Viewing Statistics
- **Average Blood Sugar**: Shows your 30-day average
- **Last Reading**: Your most recent blood sugar value
- **Medications Today**: Shows how many medications you've taken today

### Viewing History
- Scroll down to see your 7-day medication history
- View all recorded blood sugar readings
- Delete any incorrect entries

## 🔔 Alert System

The application uses audio alerts for:
- **Low Blood Sugar** (< 70 mg/dL): Red alert with sound
- **High Blood Sugar** (> 180 mg/dL): Yellow warning with sound
- **Medication Reminders**: Sound notification at scheduled time
- **Custom Reminders**: Sound notification at scheduled time

## 🔒 Security Features

- Secure Firebase Authentication
- User-specific data isolation
- Password-protected accounts
- Database rules prevent unauthorized access

## 🌐 Browser Compatibility

- ✅ Chrome (Recommended)
- ✅ Firefox
- ✅ Safari
- ✅ Edge
- ⚠️ Internet Explorer (Not supported)

## 📱 Mobile Responsive

The application is fully responsive and works on:
- Desktop computers
- Tablets
- Mobile phones

## 🛠️ Troubleshooting

### Cannot Login/Register
- Check your internet connection
- Verify Firebase configuration is correct
- Ensure Email/Password authentication is enabled in Firebase

### Data Not Saving
- Check browser console for errors
- Verify database rules are set correctly
- Ensure you're logged in

### Alerts Not Working
- Allow audio permissions in your browser
- Check browser volume settings
- Some browsers require user interaction before playing sounds

### Database Permission Denied
- Go to Firebase Console > Realtime Database > Rules
- Ensure rules match the provided configuration
- Make sure you're logged in with the correct account

## 🔄 Data Structure

The application stores data in the following structure:

```
database/
├── users/
│   └── {userId}/
│       ├── name
│       ├── email
│       └── createdAt
├── bloodSugar/
│   └── {userId}/
│       └── {readingId}/
│           ├── value
│           ├── type
│           ├── timestamp
│           ├── date
│           └── time
├── medications/
│   └── {userId}/
│       └── {medicationId}/
│           ├── name
│           ├── dosage
│           ├── time
│           ├── frequency
│           ├── createdAt
│           └── active
├── medicationIntake/
│   └── {userId}/
│       └── {intakeId}/
│           ├── medicationId
│           ├── medicationName
│           ├── dosage
│           ├── timestamp
│           ├── date
│           └── time
└── reminders/
    └── {userId}/
        └── {reminderId}/
            ├── time
            ├── description
            ├── active
            └── createdAt
```

## 💡 Tips

1. **Regular Monitoring**: Record blood sugar at consistent times for better tracking
2. **Set Multiple Reminders**: Add reminders 5 minutes before medication time
3. **Review History**: Check your 7-day history regularly to see patterns
4. **Normal Ranges**: 
   - Fasting: 70-100 mg/dL
   - Before Meal: 70-130 mg/dL
   - After Meal: < 180 mg/dL
5. **Backup**: Export your data periodically from Firebase Console

## ⚠️ Disclaimer

This application is a tracking tool and should not replace professional medical advice. Always consult with your healthcare provider for medical decisions.

## 🤝 Support

For issues or questions:
1. Check the Troubleshooting section
2. Review Firebase Console for errors
3. Check browser console for JavaScript errors

## 🔮 Future Enhancements

Potential features for future versions:
- Data export to PDF/CSV
- Graphs and charts for trends
- A1C calculator
- Meal logging
- Exercise tracking
- Integration with glucose monitors
- Multi-language support
- Dark mode

---

**Remember**: This tool helps you track your health, but always consult with your healthcare provider for medical advice and treatment decisions.
