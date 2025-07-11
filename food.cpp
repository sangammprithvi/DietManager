#include <bits/stdc++.h>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

//My compiler version is older
enum class Gender
{
    MALE,
    FEMALE,
    OTHER
};

enum class ActivityLevel
{
    SEDENTARY,
    LIGHTLY_ACTIVE,
    MODERATELY_ACTIVE,
    VERY_ACTIVE,
    EXTREMELY_ACTIVE
};

enum class CalorieCalculationMethod
{
    HARRIS_BENEDICT, //two different formulas exist for calculating per day kcal requirement for men and women
    MIFFLIN_ST_JEOR
};

class Food;
class BasicFood;
class CompositeFood;

// abstract Base: class Food. cannot be instantiated directly
class Food
{
protected:
    string name;
    vector<string> keywords;
    string type;

public:
    Food(const string &name, const vector<string> &keywords, const string &type)
        : name(name), keywords(keywords), type(type) {}

    virtual ~Food() = default;
    //always use a virtual destructor when working with inherited classes
    //because when you destroy a child class using a reference to the base class, it needs to destroy child as well as base
    
    virtual float getCalories() const = 0; //pure virtual function with no body, no implementation.
    //every subclass will have its own version of this function.
    //we made this pure virtual cuz Food class doesnt have calories of its own

    const string& getName() const { return name; }
    const vector<string>& getKeywords() const { return keywords; } //tasty, fresh, etc
    const string& getType() const { return type; } //food type, basic or compound

    virtual json toJson() const //return a json object, basically a map
    {
        json j;
        j["name"] = name;
        j["keywords"] = keywords;
        j["type"] = type;
        j["calories"] = getCalories();
        return j;
    }

    virtual void display() const //display details abt the food
    {
        cout << "Name: " << name << endl;
        cout << "Type: " << type << endl;
        cout << "Calories: " << getCalories() << endl; //calls getCalories which is a virtual function
        cout << "Keywords: ";
        for (size_t i = 0; i < keywords.size(); ++i)
        {
            cout << keywords[i];
            if (i < keywords.size() - 1)
                cout << ", ";
        }
        cout << endl;
    }
};

// Basic Food class for apple, bread etc
class BasicFood : public Food //BasicFood "is-a" Food
{
private:
    float calories;

public:
    BasicFood(const string &name, const vector<string> &keywords, float calories)
        : Food(name, keywords, "basic"), calories(calories) {} //constructor

    float getCalories() const override { return calories; } // to override getCalories from Food.

    static shared_ptr<BasicFood> fromJson(const json &j) //static function can be called even when the object doesnt exist yet
    //we can directly say -> shared_ptr<BasicFood> b = BasicFood::fromJson(j); which returns a ptr 
    //this is basically an alternate constructor from a json object
    {
        string name = j["name"];
        vector<string> keywords = j["keywords"].get<vector<string>>(); //j["keywords"] is not a vector, its a json object (array)
        float calories = j["calories"];
        return make_shared<BasicFood>(name, keywords, calories); //calls the constructor
    }
};

// Component for Composite Food
struct FoodComponent
{   //no private member "calories" here, it depends on the serving sizes
    //this isnt composite food like PB&J sandwich, its just individual components likePB&J
    shared_ptr<Food> food; 
    float servings;

    FoodComponent(shared_ptr<Food> food, float servings) : food(food), servings(servings) {}

    json toJson() const
    {   
        json j; //only two keys, name and servings. it doesnt have calories 
        j["name"] = food->getName(); //original function
        j["servings"] = servings;
        return j;
    }
};

// Composite Food class joins many componentfoods
class CompositeFood : public Food
{
private:
    vector<FoodComponent> components;

public:
    CompositeFood(const string &name, const vector<string> &keywords, const vector<FoodComponent> &components)
        : Food(name, keywords, "composite"), components(components) {}

    float getCalories() const override //pure virtual function in base class Food
    {
        float totalCalories = 0.0f;
        for (const auto &component : components)
        {
            totalCalories += component.food->getCalories() * component.servings;
        }                   //component.food is public, getCalories is defined everywhere else
        return totalCalories;
    }

    json toJson() const override
    {
        json j = Food::toJson(); //this says don't override, after this line, j containes name and keywords and type
        json componentsJson = json::array();

        for (const auto &component : components)
        {
            componentsJson.push_back(component.toJson());
        }

        j["components"] = componentsJson;
        return j;
    }

    void display() const override
    {
        Food::display(); //first display name and type and keywords and calories, no override
        //FOood::display() calls getCalories which being purely virtual calls getCalories in this subclass
        cout << "Components:" << endl;
        for (const auto &component : components)
        {
            cout << "  - " << component.food->getName()
                 << " (" << component.servings << " serving"
                 << (component.servings > 1 ? "s" : "") << ")" << endl;
        }
    }

    static shared_ptr<CompositeFood> createFromComponents( //static member function doesnt belong to any specific instance
        //can be called without creating the obj
        //doesnt have a this pointer, so it cannot access non static members
        const string &name,
        const vector<string> &keywords,
        const vector<FoodComponent> &components)
    {
        return make_shared<CompositeFood>(name, keywords, components);
    } //another constructor basically
};

// Food Database Manager class, not inherited
class FoodDatabaseManager
{
public:
    map<string, shared_ptr<Food>> foods;
    //the pointers are of "Food" class, don't know what type
private:
    string databaseFilePath;
    bool modified;

    void clear()
    {
        foods.clear();
    }

public:
    FoodDatabaseManager(const string &filePath = "food_database.json")
        : databaseFilePath(filePath), modified(false) {}

    bool loadDatabase() //loads a JSON file from the filePath, in first pass it directly loads basicfoods into the map and in 
    //second pass it deals with composite foods. returns true if loading was successful and false otherwise
    {
        clear(); //clear the map from string to food ptrs

        ifstream file(databaseFilePath);
        if (!file.is_open())
        {
            cout << "No existing database found. Starting with empty database." << endl;
            return false;
        }

        try
        {
            json j;
            file >> j;

            // Store the entire JSON data for each food
            map<string, json> pendingFoods;

            // First pass: load all basic foods and catalogue composite foods
            for (const auto &foodJson : j)
            {
                string type = foodJson["type"];
                string name = foodJson["name"];

                if (type == "basic")
                {
                    foods[name] = BasicFood::fromJson(foodJson); //map from name to food, we create the food using fromJson
                }
                else if (type == "composite") //composite foods are handled in second pass.
                {
                    pendingFoods[name] = foodJson;
                }
            }

            // Function to recursively load a composite food and its dependencies
            function<shared_ptr<Food>(const string &)> loadCompositeFood = [&](const string &name) -> shared_ptr<Food>
            {   // lambda function. returns pointer to the food after loading it.
                // If already loaded, return it
                if (foods.find(name) != foods.end())
                {
                    return foods[name]; //is a simple food
                }

                // If not a pending composite food, can't load it
                if (pendingFoods.find(name) == pendingFoods.end())//is not composite but isnt simple either
                {
                    cout << "Warning: Food '" << name << "' not found." << endl;
                    return nullptr;
                }

                // Get the food's JSON
                json foodJson = pendingFoods[name];

                // Load all components, for this particular composite food.
                vector<FoodComponent> components;
                for (const auto &componentJson : foodJson["components"])
                {
                    string componentName = componentJson["name"];
                    float servings = componentJson["servings"];

                    // Recursively load component if needed
                    shared_ptr<Food> componentFood;
                    if (foods.find(componentName) != foods.end()) //i.e component is a simple food.
                    {
                        componentFood = foods[componentName];
                    }
                    else
                    {
                        componentFood = loadCompositeFood(componentName);
                    }

                    if (componentFood)
                    {
                        components.emplace_back(componentFood, servings);
                    }
                    else
                    {
                        cout << "Warning: Component '" << componentName
                             << "' not found for composite food '" << name << "'" << endl;
                    }
                }

                // Create the composite food
                vector<string> keywords = foodJson["keywords"].get<vector<string>>();
                shared_ptr<Food> food = make_shared<CompositeFood>(name, keywords, components);

                // Add it to loaded foods
                foods[name] = food;

                return food;
            }; //lambda recursive fn ends here

            // Second pass: load all composite foods with dependencies
            for (const auto& pair : pendingFoods) //[name, ptr]
            {
                loadCompositeFood(pair.first); //name is passed as parameter to the lambda
            }

            cout << "Database loaded: " << foods.size() << " foods." << endl;
            return true;
        }
        catch (const exception &e)
        {
            cout << "Error loading database: " << e.what() << endl;
            return false;
        }
    }

    bool saveDatabase() //saves the in-memory database (foods map) to the file
    {
        try
        {
            json j = json::array(); //empty array
            //convert each Food object into JSON and add to j

            for (const auto& pair : foods) //string, ptr
            {
                j.push_back(pair.second->toJson()); //toJson is a virtual function in "Food" so it depends on whether basic or composite
            }

            ofstream file(databaseFilePath);
            if (!file.is_open())
            {
                cout << "Error: Unable to open file for writing." << endl;
                return false;
            }

            file << j.dump(4); // Pretty print with 4 spaces
            file.close();

            modified = false;
            cout << "Database saved to " << databaseFilePath << endl;
            return true;
        }
        catch (const exception &e)
        {
            cout << "Error saving database: " << e.what() << endl;
            return false;
        }
    }

    bool addFood(shared_ptr<Food> food)
    {
        string name = food->getName();
        if (foods.find(name) != foods.end())
        {
            cout << "Error: A food with name '" << name << "' already exists." << endl;
            return false;
        }

        foods[name] = food;
        modified = true;
        return true;
    }

    vector<shared_ptr<Food>> searchFoodsByKeywords(const vector<string> &keywords, bool matchall) //should we match all keywords?
    {//returns vector of ptrs to foods which have those keywords
        vector<shared_ptr<Food>> results;
        // if matchall is there, we need foods with all keywords, else food which atleast one keyword
        for (const auto  & pair : foods) //pair.first is name, pair.second is foodptr
        {
            int cnt = 0;
            for (auto &keyword : keywords)//whatever user searches for?
            {
                string lowerKeyword = keyword;
                transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower); //transform entirely to lower
                for (const auto &foodKeyword : pair.second->getKeywords())
                {
                    string lowerFoodKeyword = foodKeyword;
                    transform(lowerFoodKeyword.begin(), lowerFoodKeyword.end(), lowerFoodKeyword.begin(), ::tolower);
                    if (lowerFoodKeyword.find(lowerKeyword) != string::npos)//keyword is found within one of that foods keywordlist
                    {
                        cnt++;
                        break;
                    }
                }
            }
            if (matchall && cnt == keywords.size())
            {
                results.push_back(pair.second);
            }
            else if (!matchall && cnt > 0)
            {
                results.push_back(pair.second);
            }
        }
        return results;
    }

    shared_ptr<Food> getFood(const string &name)
    {
        auto it = foods.find(name);
        if (it != foods.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void listAllFoods() const
    {
        cout << "\n=== All Foods in Database (" << foods.size() << ") ===" << endl;
        for (const auto &pair : foods)
        {
            cout << pair.first << " (" << pair.second->getType() << ") - " << pair.second->getCalories() << " calories" << endl;
        }
        cout << "===========================" << endl;
    }

    bool isModified() const
    {
        return modified; //cuz modified is a private member
    }
};

// Food log entry for a specific day
class FoodEntry //this object is instantiated and goes into the diary
{
public:
    string foodName;
    double servings;
    double calories;

    FoodEntry(const string &name, double servs, double cals)
        : foodName(name), servings(servs), calories(cals) {}
};

// Date handling utility
class DateUtil
{//doesnt have a constructor. just has two static functions which are made public
public:
    static string getCurrentDate() //static, can be accessed anytime without creating obj
    {   //returns todays date as a string in the format YYYY-MM-DD
        auto now = chrono::system_clock::now(); //current system time point
        auto time = chrono::system_clock::to_time_t(now); //time_t is an integer type data, represents number of seconds since Jan1, 1970
        tm tm = *localtime(&time);

        stringstream ss;
        ss << put_time(&tm, "%Y-%m-%d");
        return ss.str(); //stream to string, return that string
    }

    static bool isValidDate(const string &dateStr)
    {
        if (dateStr.length() != 10) //4 for year, 2 for month, 2 for day and 2 dashes
            return false;

        // Check format: YYYY-MM-DD
        for (int i = 0; i < 10; i++)
        {
            if ((i == 4 || i == 7) && dateStr[i] != '-')
                return false;
            else if (i != 4 && i != 7 && !isdigit(dateStr[i]))
                return false;
        }

        int year = stoi(dateStr.substr(0, 4));
        int month = stoi(dateStr.substr(5, 2));
        int day = stoi(dateStr.substr(8, 2));

        if (month < 1 || month > 12)
            return false;

        int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        // Adjust for leap year
        if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0))
        {
            daysInMonth[2] = 29;
        }

        return day >= 1 && day <= daysInMonth[month];
    }
};

// Command interface for undo functionality
class Command
{
public:
    virtual ~Command() = default;
    virtual void execute() = 0; //pure virtual. Command is an abstract class, cannot be instantiated
    virtual void undo() = 0;
    virtual string getDescription() const = 0;
};

// Food diary main class
class FoodDiary
{//entries are objects of class FoodEntry
private:
    string logFile;
    map<string, vector<FoodEntry>> dailyLogs;
    stack<shared_ptr<Command>> undoStack;
    string currentDate;
    FoodDatabaseManager &dbManager;

public:
    FoodDiary(FoodDatabaseManager &db, const string &log)
        : dbManager(db), logFile(log), currentDate(DateUtil::getCurrentDate()) //log is the file of logs .json file
    {
        loadLogs();
    }

    ~FoodDiary()
    {
        saveLogs(); //always make sure to save before destroying
    }

    // Log operations
    void loadLogs()
    {
        try
        {
            ifstream file(logFile);
            if (!file.is_open())
            {
                cout << "No existing log file found. Creating a new one." << endl;
                return;
            }

            json j;
            file >> j;
            file.close();

            for (auto & item : j.items()) //each item is a map from date to entries
            {
                const string& date = item.key();
                const json& entries = item.value();
                for (const auto &entry : entries)
                {
                    string foodName = entry["food"];
                    double servings = entry["servings"];
                    double calories = entry["calories"];
                    dailyLogs[date].emplace_back(foodName, servings, calories);
                    //dailyLogs is map from string to vector<FoodEntry>
                    //emplace_back automatically calls the constructor.
                    //recall: FoodEntry has members string foodname, double servings, double calories.
                }
            }

            cout << "Loaded food logs for " << dailyLogs.size() << " days." << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error loading logs: " << e.what() << endl;
        }
    }

    void saveLogs()
    {
        try
        {
            json j;

            for (const auto & pair : dailyLogs) //dailyLogs is a map from string to vector<FoodEntry>
            {
                json dateEntries = json::array();

                for (const auto &entry : pair.second)
                {   //we do the following for each day:
                    json entryJson;
                    entryJson["food"] = entry.foodName;
                    entryJson["servings"] = entry.servings;
                    entryJson["calories"] = entry.calories;
                    dateEntries.push_back(entryJson);
                }

                j[pair.first] = dateEntries;
            }

            ofstream file(logFile);
            if (!file.is_open())
            {
                cerr << "Unable to open log file for writing: " << logFile << endl;
                return;
            }

            file << setw(4) << j;
            file.close();

            cout << "Logs saved successfully." << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error saving logs: " << e.what() << endl;
        }
    }

    // Command to add a food entry
    class AddFoodCommand : public Command //nested inherited class.
    {   //This class is nested within food diary
    private:
        FoodDiary &diary;
        string date;
        string foodName;
        double servings;
        double calories;

    public:
        AddFoodCommand(FoodDiary &d, const string &dt, const string &name, double servs)
            : diary(d), date(dt), foodName(name), servings(servs)
        {
            // Calculate calories based on food definition
            // auto it = diary.foods.find(foodName);
            auto it = diary.dbManager.getFood(foodName);
            if (it != nullptr)
            {
                calories = it->getCalories() * servings;
            }
            else
            {
                // cerr << "Food not found: " << foodName << endl;
                calories = 0;
            }
        }

        void execute() override 
        {//execute() is a pure virtual function of the Command class
            diary.dailyLogs[date].emplace_back(foodName, servings, calories); //constructs a FoodEntry
        }

        void undo() override
        {   //undo() is also a pure virtual function of the Command class
            auto &entries = diary.dailyLogs[date];
            if (!entries.empty())
            {
                // Remove the latest entry with this food name
                for (auto it = entries.rbegin(); it != entries.rend(); ++it)
                {
                    if (it->foodName == foodName && abs(it->servings - servings) < 0.001)
                    {
                        entries.erase((it + 1).base());
                        break;
                    }
                }
            }

            // If the daily log is now empty, remove the date entry
            if (entries.empty())
            {
                diary.dailyLogs.erase(date);
            }
        }

        string getDescription() const override
        {   //another pure virtual function of the Command class
            stringstream ss;
            ss << "Add " << servings << " serving(s) of " << foodName << " ("
               << calories << " calories) on " << date;
            return ss.str();
        }
    };//End of AddFoodCommand

    // Command to delete a food entry
    class DeleteFoodCommand : public Command
    {
    private:
        FoodDiary &diary; //has members:logfile (path), dbmanager, dailylogs
        string date;
        size_t index;
        FoodEntry deletedEntry; //store in case u wanna undo

    public:
        DeleteFoodCommand(FoodDiary &d, const string &dt, size_t idx)
            : diary(d), date(dt), index(idx),
              deletedEntry("", 0, 0)//constructs a FoodEntry
        {
            // Store the entry for potential undo
            auto &entries = diary.dailyLogs[date];
            if (index < entries.size()) //idx is the index of the entry to delete, in some given day
            {
                deletedEntry = entries[index];
            }
        }

        void execute() override
        {
            auto &entries = diary.dailyLogs[date];
            if (index < entries.size())
            {
                entries.erase(entries.begin() + index);
                // If the daily log is now empty, remove the date entry
                if (entries.empty())
                {
                    diary.dailyLogs.erase(date);
                }
            }
        }

        void undo() override
        {
            // Re-add the deleted entry
            diary.dailyLogs[date].push_back(deletedEntry);
        }

        string getDescription() const override
        {
            stringstream ss;
            ss << "Delete " << deletedEntry.servings << " serving(s) of "
               << deletedEntry.foodName << " from " << date;
            return ss.str();
        }
    };//end of DeleteFoodCommand

    // Date management
    void setCurrentDate(const string &date)
    {
        if (DateUtil::isValidDate(date))
        {
            currentDate = date;
            cout << "Current date set to: " << currentDate << endl;
        }
        else
        {
            cerr << "Invalid date format. Please use YYYY-MM-DD." << endl;
        }
    }

    string getCurrentDate() const
    {
        return currentDate;
    }

    // Log display
    void displayDailyLog(const string &date) const
    {
        auto it = dailyLogs.find(date); //dailyLogs is a map from string to vector<FoodEntry>
        if (it == dailyLogs.end() || it->second.empty())
        {
            cout << "No food entries for " << date << endl;
            return;
        }

        double totalCalories = 0.0;

        cout << "\nFood Log for " << date << ":\n";
        cout << setw(5) << left << "No."
             << setw(30) << left << "Food"
             << setw(15) << left << "Servings"
             << setw(15) << right << "Calories" << endl;
        cout << string(65, '-') << endl;

        int count = 1;
        for (const auto &entry : it->second)
        {
            cout << setw(5) << left << count++
                 << setw(30) << left << entry.foodName
                 << setw(15) << left << entry.servings
                 << setw(15) << right << entry.calories << endl;

            totalCalories += entry.calories;
        }

        cout << string(65, '-') << endl;
        cout << setw(50) << left << "Total Calories:"
             << setw(15) << right << totalCalories << endl;
        cout << endl;
    }

    // Command execution with undo support
    void executeCommand(shared_ptr<Command> command)
    {
        command->execute();
        undoStack.push(command);
        cout << "Executed: " << command->getDescription() << endl;
    }

    void undo()
    {
        if (undoStack.empty())
        {
            cout << "Nothing to undo." << endl;
            return;
        }

        auto command = undoStack.top();
        undoStack.pop();

        command->undo();
        cout << "Undone: " << command->getDescription() << endl;
    }

    // Food entry management
    void addFood(const string &date, const string &foodName, double servings)
    {   //add to daily logs not to dbmanager
        auto it = dbManager.getFood(foodName);
        if (!it)
        {
            cerr << "Food not found: " << foodName << endl;
            return;
        }

        auto command = make_shared<AddFoodCommand>(*this, date, foodName, servings);
        executeCommand(command);
    }

    void deleteFood(const string &date, size_t index)
    {
        auto it = dailyLogs.find(date);
        if (it == dailyLogs.end() || index >= it->second.size())
        {
            cerr << "Invalid food entry index." << endl;
            return;
        }

        auto command = make_shared<DeleteFoodCommand>(*this, date, index);
        executeCommand(command);
    }

    // User interface methods
    void addFoodToLog()
    {
        // First, let the user choose how to select a food
        cout << "\nSelect food by:\n";
        cout << "1. Browse all foods\n";
        cout << "2. Search by keywords\n";
        cout << "Choice: ";

        int choice;
        cin >> choice;
        cin.ignore();

        vector<string> foodOptions;

        if (choice == 1)
        {
            // List all foods for selection
            dbManager.listAllFoods();

            // Convert map to vector for indexing
            for (const auto & pair: dbManager.foods) //map from name to food
            {
                foodOptions.push_back(pair.first);
            }
        }
        else if (choice == 2)
        {
            cout << "Enter keywords (separated by spaces): ";
            string keywordInput;
            getline(cin, keywordInput);

            // Split input into keywords
            vector<string> keywords;
            stringstream ss(keywordInput);
            string keyword;
            while (ss >> keyword)
            {
                keywords.push_back(keyword);
            }

            if (keywords.empty())
            {
                cout << "No keywords provided." << endl;
                return;
            }

            cout << "Match: 1. All keywords or 2. Any keyword? ";
            int matchChoice;
            cin >> matchChoice;
            cin.ignore();

            bool matchAll = (matchChoice == 1);
            auto vec = dbManager.searchFoodsByKeywords(keywords, matchAll);
            for (const auto &food : vec)
            {
                foodOptions.push_back(food->getName());
            }

            if (foodOptions.empty())
            {
                cout << "No foods match the given keywords." << endl;
                return;
            }

            // Display the matching foods
            cout << "\nMatching Foods:\n";
            for (size_t i = 0; i < foodOptions.size(); i++)
            {
                cout << (i + 1) << ". " << foodOptions[i] << endl;
            }
        }
        else
        {
            cout << "Invalid choice." << endl;
            return;
        }

        // Let the user select a food
        if (foodOptions.empty()) //it wont be empty
        {
            cout << "No foods available for selection." << endl;
            return;
        }

        cout << "\nSelect food number (1-" << foodOptions.size() << "): ";
        int foodIndex;
        cin >> foodIndex;

        if (foodIndex < 1 || foodIndex > static_cast<int>(foodOptions.size()))
        {
            cout << "Invalid food selection." << endl;
            return;
        }

        string selectedFood = foodOptions[foodIndex - 1];

        // Ask for number of servings
        cout << "Enter number of servings: ";
        double servings;
        cin >> servings;
        cin.ignore();

        if (servings <= 0)
        {
            cout << "Invalid number of servings." << endl;
            return;
        }

        // Add the food to the log
        addFood(currentDate, selectedFood, servings);
    }

    void deleteFoodFromLog()
    {
        displayDailyLog(currentDate);

        auto it = dailyLogs.find(currentDate);
        if (it == dailyLogs.end() || it->second.empty())
        {
            cout << "No entries to delete." << endl;
            return;
        }

        cout << "Enter entry number to delete: ";
        int index;
        cin >> index;
        cin.ignore();

        if (index < 1 || index > static_cast<int>(it->second.size()))
        {
            cout << "Invalid entry number." << endl;
            return;
        }

        deleteFood(currentDate, index - 1);
    }

    void changeDate()
    {
        cout << "Enter date (YYYY-MM-DD): ";
        string date;
        cin >> date;
        cin.ignore();

        setCurrentDate(date);
    }

    void showUndoStack() const
    {
        if (undoStack.empty())
        {
            cout << "Undo stack is empty." << endl;
            return;
        }

        cout << "\nUndo Stack (latest first):\n";

        // Create a temporary stack to display in reverse order
        stack<shared_ptr<Command>> tempStack = undoStack;
        int count = 1;

        while (!tempStack.empty())
        {
            cout << count++ << ". " << tempStack.top()->getDescription() << endl;
            tempStack.pop();
        }

        cout << endl;
    }
    double getTotalCaloriesForDate(const string &date) const
    {
        auto it = dailyLogs.find(date);
        if (it == dailyLogs.end())
        {
            return 0.0;
        }

        double totalCalories = 0.0;
        for (const auto &entry : it->second)
        {
            totalCalories += entry.calories;
        }
        return totalCalories;
    }
};

// Class to store user's daily profile information
class DailyProfile
{
private:
    double weight; // in kg
    ActivityLevel activityLevel; //for that specific day

public:
    DailyProfile(double w = 70.0, ActivityLevel a = ActivityLevel::MODERATELY_ACTIVE)
        : weight(w), activityLevel(a) {}

    double getWeight() const { return weight; }
    void setWeight(double w) { weight = w; }

    ActivityLevel getActivityLevel() const { return activityLevel; }
    void setActivityLevel(ActivityLevel a) { activityLevel = a; }

    json toJson() const
    {
        json j;
        j["weight"] = weight;
        j["activityLevel"] = static_cast<int>(activityLevel); //enum class can be changed to int
        return j;
    }

    static DailyProfile fromJson(const json &j) //constructor of sorts
    {
        return DailyProfile(
            j["weight"].get<double>(),
            static_cast<ActivityLevel>(j["activityLevel"].get<int>()));
    }
};

// Class to represent user's unchanging profile information
class UserProfile //daily profile has weight and activity level.
{
private:
    string userId;
    Gender gender;
    double height; // in cm
    int age;
    CalorieCalculationMethod calculationMethod;
    unordered_map<string, DailyProfile> dailyProfiles; //date to dailyprofile

    // Calculate BMR using Harris-Benedict equation
    double calculateBMRHarrisBenedict(double weight) const
    {
        if (gender == Gender::MALE)
        {
            return 66.5 + (13.75 * weight) + (5.003 * height) - (6.75 * age);
        }
        else
        {
            return 655.1 + (9.563 * weight) + (1.850 * height) - (4.676 * age);
        }
    }

    // Calculate BMR using Mifflin-St Jeor equation
    double calculateBMRMifflinStJeor(double weight) const
    {
        if (gender == Gender::MALE)
        {
            return (10 * weight) + (6.25 * height) - (5 * age) + 5;
        }
        else
        {
            return (10 * weight) + (6.25 * height) - (5 * age) - 161;
        }
    }

    // Get activity multiplier based on activity level
    double getActivityMultiplier(ActivityLevel level) const
    {
        switch (level)
        {
        case ActivityLevel::SEDENTARY:
            return 1.2;
        case ActivityLevel::LIGHTLY_ACTIVE:
            return 1.375;
        case ActivityLevel::MODERATELY_ACTIVE:
            return 1.55;
        case ActivityLevel::VERY_ACTIVE:
            return 1.725;
        case ActivityLevel::EXTREMELY_ACTIVE:
            return 1.9;
        default:
            return 1.55; // Moderate activity default
        }
    }

public:
    UserProfile(
        string id = "user",
        Gender g = Gender::OTHER,
        double h = 170.0,
        int a = 30,
        CalorieCalculationMethod m = CalorieCalculationMethod::MIFFLIN_ST_JEOR) 
        : userId(id), gender(g), height(h), age(a), calculationMethod(m) {}

    // Getters and setters
    string getUserId() const { return userId; }

    Gender getGender() const { return gender; }
    void setGender(Gender g) { gender = g; }

    double getHeight() const { return height; }
    void setHeight(double h) { height = h; }

    int getAge() const { return age; }
    void setAge(int a) { age = a; }

    CalorieCalculationMethod getCalculationMethod() const { return calculationMethod; }
    void setCalculationMethod(CalorieCalculationMethod m) { calculationMethod = m; }

    // Calculate daily calorie target
    double calculateDailyCalorieTarget(const string &date)
    {
        if (dailyProfiles.find(date) == dailyProfiles.end()) //if the map is empty
        {
            // If no profile exists for this date, copy from most recent day or use default
            setDailyProfileFromMostRecent(date); //creates for that date
        }

        const DailyProfile &profile = dailyProfiles[date]; //map isnt empty
        double bmr = 0.0;

        // Calculate BMR based on selected method
        if (calculationMethod == CalorieCalculationMethod::HARRIS_BENEDICT)
        {
            bmr = calculateBMRHarrisBenedict(profile.getWeight());
        }
        else
        {
            bmr = calculateBMRMifflinStJeor(profile.getWeight());
        }

        // Apply activity multiplier
        return bmr * getActivityMultiplier(profile.getActivityLevel());
    }

    // Check if a profile exists for a specific date
    bool hasProfileForDate(const string &date) const
    {
        return dailyProfiles.find(date) != dailyProfiles.end();
    }

    // Set daily profile for a specific date
    void setDailyProfile(const string &date, const DailyProfile &profile)
    {
        dailyProfiles[date] = profile;
    }

    // Get daily profile for a specific date
    DailyProfile getDailyProfile(const string &date)
    {
        if (dailyProfiles.find(date) == dailyProfiles.end())
        {
            setDailyProfileFromMostRecent(date);
        }
        return dailyProfiles[date];
    }

    // Set profile for a date based on most recent available profile
    void setDailyProfileFromMostRecent(const string &targetDate)
    {
        // If no profiles exist yet, create a default one
        if (dailyProfiles.empty())
        {
            dailyProfiles[targetDate] = DailyProfile(); //default constructor with default weight(70kg) and activity level (moderate)
            return;
        }

        // Find the most recent date before the target date
        string mostRecentDate = "";
        for (const auto & pair : dailyProfiles)
        {
            if (pair.first <= targetDate && (mostRecentDate.empty() || pair.first > mostRecentDate))
            {
                mostRecentDate = pair.first;
            }
        }

        // If found, copy that profile; otherwise use the earliest available profile
        if (!mostRecentDate.empty())
        {
            dailyProfiles[targetDate] = dailyProfiles[mostRecentDate];
        }
        else {
            dailyProfiles[targetDate] = DailyProfile();
        }
        
    }

    json toJson() const //save profile as json
    {
        json j;
        j["userId"] = userId;
        j["gender"] = static_cast<int>(gender); //while converting to json, we store gender as an int rather than string
        j["height"] = height;
        j["age"] = age;
        j["calculationMethod"] = static_cast<int>(calculationMethod);

        json dailyProfilesJson;
        for (const auto &pair : dailyProfiles)
        {
            dailyProfilesJson[pair.first] = pair.second.toJson();
        }
        j["dailyProfiles"] = dailyProfilesJson;

        return j;
    }

    // Load profile from JSON
    static UserProfile fromJson(const json &j)
    {
        UserProfile profile( //construct
            j["userId"].get<string>(),
            static_cast<Gender>(j["gender"].get<int>()), //we store gender as an int rather than a string.
            j["height"].get<double>(),
            j["age"].get<int>(),
            static_cast<CalorieCalculationMethod>(j["calculationMethod"].get<int>()));

        if (j.contains("dailyProfiles"))
        {
            for (const auto &pair : j["dailyProfiles"].items())
            {
                profile.dailyProfiles[pair.key()] = DailyProfile::fromJson(pair.value());
            }
        }

        return profile;
    }
};

// Class to manage the user's profile and goals
class ProfileManager
{
private:
    UserProfile userProfile; //has dailyProfile as a member
    FoodDiary& foodDiary;
    string profileFilePath;

    string getActivityLevelString(ActivityLevel level) const
    {   //private fn
        switch (level)
        {
        case ActivityLevel::SEDENTARY:
            return "Sedentary";
        case ActivityLevel::LIGHTLY_ACTIVE:
            return "Lightly Active";
        case ActivityLevel::MODERATELY_ACTIVE:
            return "Moderately Active";
        case ActivityLevel::VERY_ACTIVE:
            return "Very Active";
        case ActivityLevel::EXTREMELY_ACTIVE:
            return "Extremely Active";
        default:
            return "Unknown"; //wont happen
        }
    }

    string getGenderString(Gender gender) const
    {
        switch (gender)
        {
        case Gender::MALE:
            return "Male";
        case Gender::FEMALE:
            return "Female";
        case Gender::OTHER:
            return "Other";
        default:
            return "Unknown";
        }
    }

    string getCalculationMethodString(CalorieCalculationMethod method) const
    {
        switch (method)
        {
        case CalorieCalculationMethod::HARRIS_BENEDICT:
            return "Harris-Benedict";
        case CalorieCalculationMethod::MIFFLIN_ST_JEOR:
            return "Mifflin-St Jeor";
        default:
            return "Unknown";
        }
    }

public:
    ProfileManager(FoodDiary &fd, const string &profileFile)
        : foodDiary(fd), profileFilePath(profileFile)
    {
        loadProfile();
    }

    ~ProfileManager()
    {
        saveProfile(); //always save the profile before destructing
    }

    // Load profile from file
    void loadProfile()
    {
        try
        {
            ifstream file(profileFilePath);
            if (!file.is_open())
            {
                cout << "No existing profile found. Starting with default profile." << endl;
                return;
            }

            json j;
            file >> j;
            userProfile = UserProfile::fromJson(j);

            cout << "Profile loaded successfully." << endl;
        }
        catch (const exception &e)
        {
            cout << "Error loading profile: " << e.what() << endl;
        }
    }

    // Save profile to file
    void saveProfile()
    {
        try
        {
            json j = userProfile.toJson(); //define in class UserProfile

            ofstream file(profileFilePath);
            file << j.dump(2); //serializes a JSON object to a string. 2 spaces per level (pretty printing)
            cout << "Profile saved successfully." << endl;
        }
        catch (const exception &e)
        {
            cout << "Error saving profile: " << e.what() << endl;
        }
    }

    // Display user profile
    void displayUserProfile(const string &date) //for certain date
    {
        DailyProfile dailyProfile = userProfile.getDailyProfile(date);
        cout << "\n===== User Profile for " << date << "=====" << endl;
        cout << "Gender: " << getGenderString(userProfile.getGender()) << endl;
        cout << "Height: " << userProfile.getHeight() << " cm" << endl;
        cout << "Age: " << userProfile.getAge() << " years" << endl;
        cout << "Calorie calculation method: " << getCalculationMethodString(userProfile.getCalculationMethod()) << endl;
        cout << "Weight: " << dailyProfile.getWeight() << " kg" << endl;
        cout << "Activity Level: " << getActivityLevelString(dailyProfile.getActivityLevel()) << endl;

        // Calculate and display calorie goal
        double calorieTarget = userProfile.calculateDailyCalorieTarget(date);
        cout << "Daily Calorie Target: " << calorieTarget << " calories" << endl;
        cout << "=============================" << endl;
    }

    // Display daily profile for a specific date
    void displayDailyProfile(const string &date)
    {
        DailyProfile dailyProfile = userProfile.getDailyProfile(date);

        cout << "\n===== Daily Profile for " << date << " =====" << endl;
        cout << "Weight: " << dailyProfile.getWeight() << " kg" << endl;
        cout << "Activity Level: " << getActivityLevelString(dailyProfile.getActivityLevel()) << endl;

        // Calculate and display calorie goal
        double calorieTarget = userProfile.calculateDailyCalorieTarget(date);
        cout << "Daily Calorie Target: " << calorieTarget << " calories" << endl;
    }

    // Calculate and display calorie summary for a date
    void displayCalorieSummary(const string &date)
    {
        // Get calorie target
        double calorieTarget = userProfile.calculateDailyCalorieTarget(date);

        // Calculate consumed calories
        double consumedCalories = 0.0;

        // This assumes your LogManager class has a method to get the total calories for a date
        // Modify this part based on your LogManager implementation
        consumedCalories = foodDiary.getTotalCaloriesForDate(date);

        // Calculate difference
        double calorieDifference = consumedCalories - calorieTarget;

        cout << "\n===== Calorie Summary for " << date << " =====" << endl;
        cout << "Target: " << calorieTarget << " calories" << endl;
        cout << "Consumed: " << consumedCalories << " calories" << endl;

        if (calorieDifference < 0)
        {
            cout << "Remaining: " << -calorieDifference << " calories" << endl;
        }
        else
        {
            cout << "Excess: " << calorieDifference << " calories" << endl;
        }
    }

    // Update user profile
    void updateUserProfile(const string &date)
    {
        cout << "\n===== Update User Profile for " << date << "=====" << endl;

        cout << "Enter age: ";
        int age;
        cin >> age;
        if (age < 0 || age > 1000) {
            cout << "Invalid age. Please enter a valid age." << endl;
            return;
        }
        
        cout << "Enter weight (kg): ";
        double weight;
        cin >> weight;
        if (weight <= 0) {
            cout << "Invalid weight. Please enter a valid weight." << endl;
            return;
        }
        
        DailyProfile dailyProfile = userProfile.getDailyProfile(date);
        
        cout << "Select activity level (0 = Sedentary, 1 = Lightly Active, "
        << "2 = Moderately Active, 3 = Very Active, 4 = Extremely Active): ";
        int activityChoice;
        cin >> activityChoice;
        if (activityChoice < 0 || activityChoice > 4) {
            cout << "Invalid activity level. Please select a valid option." << endl;
            return;
        }
        
        
        userProfile.setAge(age);
        dailyProfile.setWeight(weight);
        dailyProfile.setActivityLevel(static_cast<ActivityLevel>(activityChoice));
        userProfile.setDailyProfile(date, dailyProfile);

        cout << "Select calorie calculation method (0 = Harris-Benedict, 1 = Mifflin-St Jeor): ";
        int methodChoice;
        cin >> methodChoice;
        userProfile.setCalculationMethod(static_cast<CalorieCalculationMethod>(methodChoice));

        cin.ignore();
    }

    // Update daily profile for a specific date
    void updateDailyProfile(const string &date)
    {
        DailyProfile dailyProfile = userProfile.getDailyProfile(date);

        cout << "\n===== Update Daily Profile for " << date << " =====" << endl;

        cout << "Enter weight (kg): ";
        double weight;
        cin >> weight;
        dailyProfile.setWeight(weight);

        cout << "Select activity level (0 = Sedentary, 1 = Lightly Active, "
             << "2 = Moderately Active, 3 = Very Active, 4 = Extremely Active): ";
        int activityChoice;
        cin >> activityChoice;
        dailyProfile.setActivityLevel(static_cast<ActivityLevel>(activityChoice));

        userProfile.setDailyProfile(date, dailyProfile);

        cin.ignore();
    }

    // Change calculation method
    void changeCalculationMethod()
    {
        cout << "\n===== Change Calculation Method =====" << endl;
        cout << "Current method: " << getCalculationMethodString(userProfile.getCalculationMethod()) << endl;
        cout << "Available methods:" << endl;
        cout << "0 - Harris-Benedict" << endl;
        cout << "1 - Mifflin-St Jeor" << endl;
        cout << "Select method: ";

        int methodChoice;
        cin >> methodChoice;
        userProfile.setCalculationMethod(static_cast<CalorieCalculationMethod>(methodChoice));

        cout << "Calculation method changed to "
             << getCalculationMethodString(userProfile.getCalculationMethod()) << endl;

        cin.ignore();
    }
};


// Command Line Interface class
class DietAssistantCLI
{
private:
    FoodDatabaseManager dbManager;
    FoodDiary foodDiary;
    ProfileManager profileManager;
    bool running;

    void displayMenu()
    {
        cout << "\n===== Diet Assistant Menu =====\n";
        cout << "1. Search foods\n";
        cout << "2. View food details\n";
        cout << "3. Add basic food\n";
        cout << "4. Create composite food\n";
        cout << "5. List all foods\n";
        cout << "6. Save database\n";
        cout << "7. View Today's Log\n";
        cout << "8. Add Food Entry\n";
        cout << "9. Delete Food Entry\n";
        cout << "10. Change Current Date\n";
        cout << "11. Undo Last Action\n";
        cout << "12. View User Profile\n";
        cout << "13. Update User Profile\n";
        cout << "14. Change calorie calculation method\n";
        cout << "15. View Calorie summary\n";
        cout << "16. Exit\n";
        cout << "==============================\n";
        cout << "Enter choice (1-17): ";
    }

    void searchFoods()
    {
        cout << "1. Do you want to search by keywords? (yes/no): ";
        string choice;
        cin >> choice;
        if (choice == "yes")
        {
            cout << "Enter keywords (separated by spaces): ";
            string keywordInput;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin, keywordInput);

            // Split input into keywords
            vector<string> keywords;
            stringstream ss(keywordInput);
            string keyword;
            while (ss >> keyword)
            {
                keywords.push_back(keyword);
            }

            if (keywords.empty())
            {
                cout << "No keywords provided." << endl;
                return;
            }

            cout << "Match: 1. All keywords or 2. Any keyword? ";
            int matchChoice;
            cin >> matchChoice;

            bool matchAll = (matchChoice == 1);
            auto vec = dbManager.searchFoodsByKeywords(keywords, matchAll);
            for (const auto &food : vec)
            {
                cout << food->getName() << " (" << food->getType() << ") - "
                     << food->getCalories() << " calories" << endl;
            }
        }
        else
        {
            cout << "Enter food name: ";
            string name;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin, name);

            shared_ptr<Food> food = dbManager.getFood(name);
            if (food)
            {
                cout << "\n=== Food Details ===" << endl;
                food->display();
            }
            else
            {
                cout << "Food '" << name << "' not found." << endl;
            }
        }
    }

    void viewFoodDetails()
    {
        cout << "\nEnter food name: ";
        string name;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        getline(cin, name);

        shared_ptr<Food> food = dbManager.getFood(name);
        if (food)
        {
            cout << "\n=== Food Details ===" << endl;
            food->display();
        }
        else
        {
            cout << "Food '" << name << "' not found." << endl;
        }
    }

    void addBasicFood()
    {
        string name;
        vector<string> keywords;
        float calories;

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        cout << "\n=== Add Basic Food ===" << endl;

        cout << "Enter food name: ";
        getline(cin, name);

        cout << "Enter calories per serving: ";
        cin >> calories;

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        cout << "Enter keywords (comma-separated): ";
        string keywordsStr;
        getline(cin, keywordsStr);

        // Parse comma-separated keywords
        size_t pos = 0;
        string token;
        while ((pos = keywordsStr.find(',')) != string::npos)
        {
            token = keywordsStr.substr(0, pos);
            token.erase(0, token.find_first_not_of(' '));
            token.erase(token.find_last_not_of(' ') + 1);
            if (!token.empty())
                keywords.push_back(token);
            keywordsStr.erase(0, pos + 1);
        }
        // Add the last keyword
        keywordsStr.erase(0, keywordsStr.find_first_not_of(' '));
        keywordsStr.erase(keywordsStr.find_last_not_of(' ') + 1);
        if (!keywordsStr.empty())
            keywords.push_back(keywordsStr);

        auto newFood = make_shared<BasicFood>(name, keywords, calories);
        if (dbManager.addFood(newFood)) //addFood returns a boolean
        {
            cout << "Basic food '" << name << "' added successfully." << endl;
        }
    }

    void createCompositeFood()
    {
        string name;
        vector<string> keywords;
        vector<FoodComponent> components;

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        cout << "\n=== Create Composite Food ===" << endl;

        cout << "Enter composite food name: ";
        getline(cin, name);

        cout << "Enter keywords (comma-separated): ";
        string keywordsStr;
        getline(cin, keywordsStr);

        // Parse comma-separated keywords
        size_t pos = 0;
        string token;
        while ((pos = keywordsStr.find(',')) != string::npos)
        {
            token = keywordsStr.substr(0, pos);
            token.erase(0, token.find_first_not_of(' '));
            token.erase(token.find_last_not_of(' ') + 1);
            if (!token.empty())
                keywords.push_back(token);
            keywordsStr.erase(0, pos + 1);
        }
        // Add the last keyword
        keywordsStr.erase(0, keywordsStr.find_first_not_of(' '));
        keywordsStr.erase(keywordsStr.find_last_not_of(' ') + 1);
        if (!keywordsStr.empty())
            keywords.push_back(keywordsStr);

        bool addingComponents = true;
        while (addingComponents)
        {
            cout << "\nEnter component food name (or 'done' to finish): ";
            string componentName;
            getline(cin, componentName);

            if (componentName == "done")
            {
                addingComponents = false;
                continue;
            }

            shared_ptr<Food> componentFood = dbManager.getFood(componentName);
            if (!componentFood)
            {
                cout << "Food '" << componentName << "' not found." << endl;
                continue;
            }

            float servings;
            cout << "Enter number of servings: ";
            cin >> servings;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            components.emplace_back(componentFood, servings);
            cout << "Added " << servings << " serving" << (servings > 1 ? "s" : "")
                 << " of '" << componentName << "'" << endl;
        }

        if (components.empty())
        {
            cout << "No components added. Composite food creation cancelled." << endl;
            return;
        }

        auto newFood = CompositeFood::createFromComponents(name, keywords, components);
        if (dbManager.addFood(newFood))
        {
            cout << "Composite food '" << name << "' created successfully." << endl;
            cout << "Total calories: " << newFood->getCalories() << endl;
        }
    }

    void handleExit() //if dbmanager is modified, it saves and exits. else just exits
    {
        if (dbManager.isModified())
        {
            cout << "Database has unsaved changes. Save before exit? (y/n): ";
            char choice;
            cin >> choice;

            if (choice == 'y' || choice == 'Y')
            {
                dbManager.saveDatabase();
            }
        }

        running = false;
    }

public:
    DietAssistantCLI(const string &databasePath = "food_database.json", const string &logPath = "food_log2.json", const string &profilePath = "user_profile2.json")
        : dbManager(databasePath), foodDiary(dbManager, logPath), profileManager(foodDiary, profilePath), running(false)
    {
    }

    void start()
    {
        running = true;
        dbManager.loadDatabase();

        cout << "Welcome to Diet Assistant!" << endl;

        while (running)
        {
            displayMenu();

            int choice;
            cin >> choice;

            switch (choice)
            {
            case 1:
                searchFoods();
                break;
            case 2:
                viewFoodDetails();
                break;
            case 3:
                addBasicFood();
                break;
            case 4:
                createCompositeFood();
                break;
            case 5:
                dbManager.listAllFoods();
                break;
            case 6:
                dbManager.saveDatabase();
                break;
            case 7:
                foodDiary.displayDailyLog(foodDiary.getCurrentDate());
                break;
            case 8:
                foodDiary.addFoodToLog();
                break;
            case 9:
                foodDiary.deleteFoodFromLog();
                break;
            case 10:
                foodDiary.changeDate();
                break;
            case 11:
                foodDiary.undo();
                break;
            case 12:
                profileManager.displayUserProfile(foodDiary.getCurrentDate());
                break;
            case 13:
                profileManager.updateUserProfile(foodDiary.getCurrentDate());
                break;
            case 14:
                profileManager.changeCalculationMethod();
                break;
            case 15:
                profileManager.displayCalorieSummary(foodDiary.getCurrentDate());
                break;
            case 16:
                handleExit();
                break;
            default:
                cout << "Invalid choice. Please try again." << endl;
            }
        }

        cout << "Thank you for using Diet Assistant. Goodbye!" << endl;
    }
};

int main()
{
    DietAssistantCLI dietAssistant;
    dietAssistant.start();
    return 0;
}