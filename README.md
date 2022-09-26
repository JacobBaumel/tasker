# Tasker v1.0.0-alpha

### A free and open source task management tool
Tasker (name to be changed soon) is a system designed to replicate web apps such as monday.com, but as a free, open source version that has no payment plans, and can be run completely on your own. 

**This is still an alpha project, and everything is subject to change!!**

Tasker runs on nothing but a client, a MySQL server, and takes up minimal storage space and RAM.

Tasker:
 - Is and will remain open source
 - Will continue to have a native version available for all/most operating systems
 - Can be used however you like
 - Does require a server host (details to come)
 - Free!
 - (Eventually) just as good as professional software

## Usage
### Definitions
 - Workspace: The overall server/environment where data is stored
 - Category: A large, overarching group of tasks to be completed
 - Task: Individual tasks with 2 text fields and a selector for current status
 - Status: The current state of a task at a specific time
 - Internal names:
	 - Connection: The IP address + login information for the main mysql server
	 - Database: The workspace within the connection, separated out because different databases inside a single mysql server represent different workspaces.
	 - Supertask: Internal name for category
	 - Stati: Plural forms of "status" (I didn't know the actual plural form so I made my own lol)
### Creating/Selecting Workspace
The main screen of Tasker will show all available workspaces present in an external JSON file (config.json). To add a new workspace, click on the "Add" button, and enter connection details. Next, if the workspace already exists, use the "Add Existing" tab to add it, and if not, the "Create New" tab to create a new workspace. A tile will appear for the new Workspace. Tiles do not appear for new connections.

### Using a workspace
The main area of the workspace is for the different categories, which will appear if any are present. Categories can be added using the "New Category" button, and deleting using the "X". Tasks will be listed under the category banner, and can be modified by clicking on their text fields, or by interacting with the dropdown. A new task can be created using the empty task template, and submitted with the checkmark.

### Using on windows:
Download the compiled binary, and save it wherever. Along with the file, OpenGL3 and GLFW3 need to be installed. OpenGL usually comes installed by default, but GLFW will need to be installed. Download the GLFW precompiled binary from [the GLFW website](https://www.glfw.org/download), extract the archive, and copy the file `glfw3.dll` from the `glfw-3.3.8.bin.WIN64/lib-mingw-w64` folder into the same directory as the Tasker binary. Additionally, download the mysql c++ connector installer for windows from [the mysql website](https://dev.mysql.com/downloads/connector/cpp/).

### Using on Linux
Along with the binary file, the packages `libopengl0` and `libglfw3` need to be installed. Additionally, download the mysql c++ connector archive for your OS from [the mysql website](https://dev.mysql.com/downloads/connector/cpp/), extract the archive, grab the `libmysqlcppconn.so` file from the `lib64` folder, and place it in the same directory the Tasker binary. In the future, there will be installers for the major linux distributions, and for windows. However, these will most likely not come until beta releases, or release candidates.

## Getting a server
As of now, any MySQL server will do, as long as at least 1 database can be accessed for a workspace. There are many free options, but one that provides full control over the server is the [Oracle Free server hosting tier](https://cloud.oracle.com), which provides full access to a dedicated server. 

After installing mysql onto the system, setup a user that can be accessed from any IP, with whatever name and password you like. Through this user, tasker will update the server. 

## Compiling
Tasker depends on 4 libraries to function: `GLFW3`, `OpenGL`, [JSON11](https://github.com/davidmoreno/json11) by David Moreno, and [Dear Imgui](https://github.com/ocornut/imgui) by Ocornut. Ensure the header files and dynamic libraries are available for `GLFW` and `OpenGL`, and edit the `INCLUDES` variable in the makefile to add the location of the header files. Additionally, if `OpenGL` or `GLFW` are installed in non-typical locations, ensure to add the proper -L flags to the `LIBS` variable in the makefile.

Clone the repositories for `JSON11` and `Dear Imgui`, and fill in the root paths in the makefile. By running make, the project should build without errors, with `.o` files going in the specified build directory, and the compiled binary to the folder with the makefile.

## Contributing
As of now, I have no dire need for more help, but if you have a great idea or a contribution you _want_ to make, the feel free to open an issue or PR. I will update the Github Project Page frequently to show what I am currently working on, but if you are absolutely _dying_ to do something, then I would say I need the most help in the UI department. Design was/is never my strong suit, so if you have some ideas to improve it, feel free to share!

Additionally, this is my first big, solo project, so if you are browsing the code and see something truly horrendous, please let me know! I will do my best to fix it to my ability.

## Todo list:
### Big things:
 - [ ] Rework backend to have a separate server binary, to have:
   - [ ] Better control over users and permissions
   - [ ] Logging/tracking who makes changes
   - [ ] "Guest" users
   - [ ] Improve functionality of "People" field, by having a selector
 - [ ] Significant UI improvements (it looks kinda bad ngl)
 - [ ] Moving server requests to a seperate thread
 - [ ] Installers
 - [ ] Figuring out how to get macos builds
 - [ ] General code cleanup
 - [ ] Preferences and customization of UI

### Small things
 - [ ] Convert all pointers to smart pointers
 - [ ] Labels on what the different fields of tasks I invisioned would represent
 - [x] Get a decent github issues/projects setup
 - [ ] Keyboard navigation
 - [ ] Rearrange some stuff to put networking into a shared/dynamic library


### Wishlist features
 - [ ] Automations???
 - [ ] Integrations???
 - [ ] Timeline???
 - [ ] Web assembly version???