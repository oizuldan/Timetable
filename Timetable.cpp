#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <map>
#include <string>

using namespace std;

// Course info
class Course
{
public:
    string name; // Name of course
    string professor; // Name of professor
    string faculty; // Who is taking this course, e.g. 2_year_ECE

    vector<string> types; // Vector which contains types of lecture (L/T/Lab) this course has
};

// Group info
class Group
{
public:
    string id; // E.g. G10
    int people; // Number of people in this group

    string faculty; // Year and faculty, e.g. 3_year_EME
    Group(string a, int b, string c)
    {
        id = a;
        people = b;
        faculty = c;
    }
};

// Room info
class Room
{
public:
    string name; // Name of the room
    int capacity; // Capacity
    string type; // Type of room, which is same as the type of course (L/T/Lab/C)
};

vector<Course> courseList; // vector of courses
vector<vector<int> > courseClasses; // for each course, we have a vector of sections

vector<Group> groupList; // vector of groups
vector<Room> roomList; // vector of rooms

vector<vector<int> > isRoomBusy; // for each room, contains a vector of 20 elements for each time, e.g. isRoomBusy[0][1] contains if 0th room is busy in Monday 11:00
vector<vector<bool> > isGroupBusy;

vector<int> classList; // list of classes (sections)
vector<int> classSize; // size of class
vector<vector<int> > classRoom; // for each class, contains a vector for each type which is room
vector<vector<int> > classTime; // for each class, contains a vector for each type which is time
vector<vector<int> > classGroups; // for each class

vector<map<string, int> > took; // took[group][class] = true/false

// Getting course info from file named fname
void inputCourses(string fname)
{
    ifstream read(fname.c_str()); // make an input stream from file called fname
    for (;;)
    {
        Course course;
        // Read until end of file
        if (!(read >> course.name >> course.professor >> course.faculty))
            break;
        // Read three types of this course
        string str;
        for (int cnt = 0; cnt < 3; cnt++)
        {
            read >> str;
            if (str == "T")
            {
                course.types.push_back("tutorial");
            }
            else if (str == "C")
            {
                course.types.push_back("complab");
            }
            else if (str == "Lab")
            {
                course.types.push_back("laboratory");
            }
        }
        courseList.push_back(course);
    }
}

// Getting groups info from file
void inputGroups(string fname)
{
    ifstream read(fname.c_str());
    string str;
    string curFaculty;
    while (read >> str)
    {
        if (str[str.size() - 1] == ':')
        {
            curFaculty = str.substr(0, str.size() - 1);
        }
        else
        {
            int cap;
            read >> cap;
            groupList.push_back(Group(str, cap, curFaculty));
        }
    }
}

// getting rooms info from file
void inputRooms(string fname)
{
    ifstream read(fname.c_str());
    Room room;
    while (read >> room.name >> room.capacity >> room.type)
    {
        roomList.push_back(room);
    }
}

// function which tries to add section of class with id cid
// section has a room for each lecture, lab, comp
// also checks if rooms has a capacity not less than ppl
bool addClass(int cid, int ppl)
{
    vector<int> rv, tv;
    bool success = true;
    // Go through all types of this course (L/Lab etc.)
    for (int i = 0; i < courseList[cid].types.size(); i++)
    {
        bool found = false;
        // Go through all rooms
        for (int room = 0; room < roomList.size(); room++)
        {
            // If this is not the room we need or it we can't fit, then skip
            if (roomList[room].type != courseList[cid].types[i])
                continue;
            if (roomList[room].capacity < ppl)
                continue;
            // For each possible time
            for (int t = 0; t < 20; t++)
            {
                // Check if room is not empty at this time, if not - then assign this room to this class
                if (!isRoomBusy[room][t])
                {
                    isRoomBusy[room][t] = cid + 1;
                    rv.push_back(room);
                    tv.push_back(t);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (!found)
        {
            success = false;
        }
    }
    if (success)
    {
        // If we have found rooms, then assign
        courseClasses[cid].push_back(classList.size());

        classList.push_back(cid);
        classSize.push_back(0);
        classRoom.push_back(rv);
        classTime.push_back(tv);
        classGroups.push_back(vector<int>());
        for (int i = 0; i < rv.size(); i++)
        {
            isRoomBusy[rv[i]][tv[i]] = cid + 1;
        }
    }
    else
    {
        // Write some Error if we haven't found any class
        // Actually this line never should complete
        cout << "ERROR occured in adding class!\n";
        exit(0);
    }
    return success;
}

// function that finds section of course cid from group with id gid
bool findClass(int gid, int cid)
{
    bool flag = 0;
    // For each class
    for (int i = 0; i < courseClasses[cid].size(); i++)
    {
        int section = courseClasses[cid][i];
        bool canTake = true;
        // Check if we can fit by size
        for (int j = 0; j < classRoom[section].size(); j++)
            canTake &= (classSize[section] + groupList[gid].people <= roomList[classRoom[section][j]].capacity);
        // Check if we aren't busy in times of this class
        for (int j = 0; j < classTime[section].size(); j++)
            canTake &= (!isGroupBusy[gid][classTime[section][j]]);
        // If we can take this class, then assign
        if (canTake)
        {
            // Group gid took this course
            took[gid][courseList[cid].name] = section;
            classSize[section] += groupList[gid].people;
            for (int j = 0; j < classTime[section].size(); j++)
                isGroupBusy[gid][classTime[section][j]] = true;
            flag = 1;
            break;
        }
    }
    return flag;
}

vector<int> lectureTime;
vector<int> lectureRoom;

// This function assigns lecture rooms to some particular class
void findLectureTime(int cid)
{
    bool flag = 0;
    // Go through all rooms
    for (int room = 0; room < roomList.size(); room++)
    {
        // and all possible times
        for (int t = 0; t < 20; t++)
        {
            bool noGroupIntersection = true;
            for (int gid = 0; gid < groupList.size(); gid++)
                if (took[gid].count(courseList[cid].name))
                    noGroupIntersection &= !isGroupBusy[gid][t];
            // if no group taking this course is busy
            if (noGroupIntersection)
            {
                // if room is not busy and tutorial/lecture
                if ((roomList[room].type == "tutorial" || roomList[room].type == "lecture") && !isRoomBusy[room][t])
                {
                    // assign this room to this course
                    isRoomBusy[room][t] = cid + 1;
                    for (int i = 0; i < courseClasses[cid].size(); i++)
                    {
                        lectureRoom[courseClasses[cid][i]] = room;
                        lectureTime[courseClasses[cid][i]] = t;
                    }
                    flag = 1;
                    break;
                }
            }
        }
        if (flag) break;
    }
    if (!flag)
    {
        exit(0);
    }
}

const string week[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};

// start & end times
const string st[] = {"09:00", "11:00", "14:00", "16:00"};
const string et[] = {"11:00", "13:00", "16:00", "18:00"};

// id is number in [0, 20)
// this function converts id to week of day and time
string conv(int id)
{
    return week[id/4] + " (" + st[id%4] + "-" + et[id%4] + ")";
}

int main()
{
    // get inputs
    inputCourses("data.txt");
    inputGroups("group.txt");
    inputRooms("rooms.txt");
    ofstream cout("Timetable.txt");
    // initialize vector sizes
    isRoomBusy.resize(roomList.size());
    for (int i = 0; i < roomList.size(); i++)
    {
        isRoomBusy[i].resize(20);
    }
    isGroupBusy.resize(groupList.size());
    for (int i = 0; i < groupList.size(); i++)
    {
        isGroupBusy[i].resize(20);
    }
    courseClasses.resize(courseList.size());
    took.resize(groupList.size());

    // for each group and course, search section
    // while it doesn't exist, create new section
    for (int gid = 0; gid < groupList.size(); gid++)
    {
        for (int cid = 0; cid < courseList.size(); cid++)
        {
            // If group gid should take course cid, then try to find course for this group
            if (courseList[cid].faculty == groupList[gid].faculty && !took[gid].count(courseList[cid].name))
            {
                while (!findClass(gid, cid))
                    addClass(cid, groupList[gid].people);
            }
        }
    }

    // Almost same for lectures
    // We do separately for lectures because they should be held together for everyone
    lectureTime.resize(classList.size());
    lectureRoom.resize(classList.size());
    for (int cid = 0; cid < courseList.size(); cid++)
        findLectureTime(cid);

    // Print an answer as a list of courses for each group
    for (int gid = 0; gid < groupList.size(); gid++)
    {
        cout << "--------\n";
        cout << "List of courses for " << groupList[gid].faculty << ' ' << groupList[gid].id << ":\n\n";
        map<string, int>::iterator it = took[gid].begin();
        for (it = took[gid].begin(); it != took[gid].end(); it++)
        {
            int section = it->second;
            cout << it->first << " | " << courseList[classList[section]].professor << endl; // Print name of course and professor's name

            // Lecture information
            cout << "    lecture: ";
            cout << roomList[lectureRoom[section]].name << ' ' << conv(lectureTime[section]) << endl;
            // Information for other types
            for (int i = 0; i < classTime[section].size(); i++)
            {
                cout << "    " << courseList[classList[section]].types[i] << ": ";
                cout << roomList[classRoom[section][i]].name << ' ';
                cout << conv(classTime[section][i]);
                cout << endl;
            }
        }
        cout << endl;
    }

    return 0;
}
