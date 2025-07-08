How to Run:
1. First compile using g++ food.cpp
2. then run ./a.out to start the application.
3. The program depends on the nlohmann\_json library for JSON parsing. This is included as a header-only library (json.hpp).

The commands from 1-6 are for managing the food database
The commands from 7-12 are for maintaining logs (undo is for logs)
The rest are for managing user profile.
We can change the date using command 12 and test the daywise profile functionality of user profile.


Program Features

Food Database Management

1.  Loading the Database:

       The program attempts to load a food database from `food_database.json` upon startup.
       If the file does not exist, an empty database is created.
2.  Adding Basic Foods:

       From the main menu, select the option to add a new food.
       Provide the food's name, keywords (separated by commas), and calorie count.
3.  Adding Composite Foods:

       Choose the option to add a new composite food.
       Enter the composite food's name and keywords.
       Add components by specifying the name of an existing food in the database and the number of servings.
4.  Searching for Foods:

       Use the search option to find foods by keywords.
       You can search for foods that match all or any of the specified keywords.
5.  Listing All Foods:

       Select the option to list all foods in the database to see a summary of each food.
6.  Saving the Database:

       Choose the option to save the current database state to `food_database.json`.

Food Diary Logging

1.  Adding Food Entries:

       Select the option to add a food entry to the diary.
       Enter the name of the food and the number of servings consumed.
       The program will automatically record the date and calculate the total calories based on the food's calorie information in the database.
2.  Viewing Daily Logs:

       Choose the option to view the food log for a specific date or the current date.
       The log displays all food entries for the selected date, including the food name, servings, and total calories.
3.  Viewing Calorie Summary:
       Select the option to view the total calories consumed on a particular day.
4.  Loading Logs:

       The program loads food logs from `food_log.json` on startup.
5.  Saving Logs:

       The program saves food logs to `food_log.json` when exiting.
6.  Undoing Actions:

       The program supports undoing the last add food entry action.
       Select the undo option to revert the last entry.

Personalized Calorie Recommendations

1.  Setting User Profile:

       Select the option to set up your user profile.
       You will be prompted to enter your gender, age, height, weight, and activity level.
       Choose a calorie calculation method (Harris-Benedict or Mifflin St Jeor).
2.  Calculating Calorie Needs:

       The program calculates your daily calorie needs based on the entered profile information and the selected calculation method.
       This provides a baseline for managing your calorie intake using the food diary.

Additional Features

   Date Handling:
       The program uses the current date for logging, but it also validates date inputs to ensure they are in the correct format (YYYY-MM-DD).
   Searching Foods By Keywords:
       The program allows searching the food by keywords
   Configuration:
       Database file path: `food_database.json`
       Log file path: `food_log.json`



