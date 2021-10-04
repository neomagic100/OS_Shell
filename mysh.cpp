/* mysh.cpp - Shell program to execute a specific set of commands
 * 
 * Created by: Michael Bernhardt
 * Last updated: Oct 3, 2021
 * 
 * To compile - in bash shell, enter the following:
 * 			 g++ mysh.cpp -o mysh
 * 
 * To run - in bash shell, enter the following:
 * 			./mysh
 */

#include <iostream>
#include <fstream>
#include <stack>
#include <vector>
#include <regex>
#include <list>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <filesystem>
#include <string>
#include <cstdio>

#define BUFFER_MAX 1024

using namespace std;

// Give each valid command an integer representation
typedef enum { movetodir_sym = 0, whereami_sym, history_sym, byebye_sym, replay_sym, start_sym,
		background_sym, dalek_sym, repeat_sym, dalekall_sym,
} command_syms;

/*
 * Class: Command - Keep track of information for a given input command
 */
class Command {
	public:

	const vector<string> KEYWORDS = {"movetodir", "whereami", "history", "byebye", "replay", "start",
							"background", "dalek", "repeat", "dalekall"};
	string cmdInput;
	string command;
	vector<string> tokenized{};
	vector<string> args{};
	int numTokens, replayNum, commandNum;
	

	// Constructor
	// Input: String input - string read from input
	Command(string input) {
		cmdInput = input;
		tokenize();
		numTokens = tokenized.size();
		command = tokenized[0];
		getCommandNum();
		getArgs();
	}

	void printHistory(vector<Command>);

	// Tokenize the input string
	void tokenize(){
		int len = cmdInput.length();
		char arr[len + 1];
		strcpy(arr, cmdInput.c_str());
		char* token;
		char* rest = arr;

		while ((token = strtok_r(rest, " ", &rest))) {
			if (token) {
				string s = token;
				tokenized.insert(tokenized.end(), s);
			}
		}
	}

	// Combine the arguments into a single string
	// Output: a string of arguments
	string combineArgs () {
		string combinedArgs = "";
		if (numTokens < 2)
			return combinedArgs;

		int len = args.size();
		for (int i = 0; i < len; i++)
			combinedArgs += args[i] + " ";

		return combinedArgs;
	}

	// Test if the command is a valid recognized command
	// Output: bool - True: is valid
	bool validCmd() {
		return (commandNum >= 0);
	}
	
	// Test if the command has arguments
	// Output: bool - True: has arguments
	bool hasArgs() {
		return !args.empty();
	}
	
	// Test if the argument has a given string
	// Input: String arg - a string 
	// Output: bool - True: is in arg
	bool argsHas(string arg) {
		int len = args.size();
		for (int i = 0; i < len; i++) {
			if (args[i] == arg)
				return true;
		}
	}
	
	// Test if the first argument is a given string
	// Input: String arg - a string 
	// Output: bool - True: is in arg
	bool argsIs(string arg) {
		return args[0] == arg;
	}

	private:

	// Get and Set the commandNum based on the array of keywords
	void getCommandNum() {
		for (int i = 0; i < KEYWORDS.size(); i++) {
			if (KEYWORDS[i] == command) {
				commandNum = i;
				return;
			}
		}
		commandNum = -1;
	}
	
	// Get and set the args of the command
	void getArgs() {
		for (int i = 1; i < numTokens; i++)
			args.insert(args.end(), tokenized[i]);
	}
};

/*
 * Class: Command_Stack - Keep track of the history of valid commands entered in the shell
 */
class Command_Stack {
	private:

	list<Command> historyStack;
	string filename = "mysh_history.txt";

	public:

	// Print the current history stack to the console while setting the replay numbers for each command
	void printHistory() {
		list<Command>::iterator it;
		int iter = 0;
		int size = historyStack.size();
		if (size == 0) return;
		cout << "History:" << endl;
		for (it = historyStack.begin(); it != historyStack.end(); it++) {
			it->replayNum = iter;
			cout << iter << ": " << it->cmdInput << endl;
			iter++;
		}
	}
	
	// Get the replay number of a command in the stack
	// Input: Command cmd - a Command to find in stack
	// Output: the Command found, or Command("0") if not
	Command findReplayNum(Command cmd) {
		list<Command>::iterator it;
		int size = historyStack.size();
		if (size == 0) return Command("0");
		for (it = historyStack.begin(); it != historyStack.end(); it++) {
			int replayArg = stoi(cmd.args[0]);
			if (replayArg == it->replayNum) {
				if (it->commandNum == replay_sym) {
					cout << "Invalid Command: " << cmd.command << " " << it->cmdInput << endl;
					break;
				}
				return *it;
			}
		}
		
		return Command("0");
	}

	// Read the history stack from a file
	void readFromFile() {
		ifstream fin(filename);
		if (!fin.is_open()) return; // no file yet

		string line, cmd;
		list<string> cmdsIn;

		if (fin.good()) {
			while (getline(fin, line)) {
				stringstream stream(line);

				while (getline(stream, cmd, ',')) {
					cmdsIn.push_back(cmd);
				}
			}
		}

		list<string>::iterator it;
		for (it = cmdsIn.begin(); it != cmdsIn.end(); it++) {
			historyStack.push_back(Command(*it));
		}
	}

	// Print the current history stack to file
	void printToFile() {
		int numLines = historyStack.size();
		if (numLines == 0) return;

		list<Command>::iterator it;
		ofstream fout;
		fout.open(filename);
		int i = 0;
		for (it = historyStack.begin(); it != historyStack.end(); it++) {
			fout << it->cmdInput;
			if (i + 1 < numLines)
				 fout << ',';
			i++;
		}
		fout << endl;
		fout.close();
	}

	// Get the size of the history stack
	int size() {
		return historyStack.size();
	}

	// Clear current history stack and delete history file
	void clearHistory() {
		while (!historyStack.empty())
			historyStack.pop_front();
		remove(filename.c_str());
		cout << "History Cleared" << endl;
	}

	// Push a command onto the history stack
	// Input: Command c - a Command
	void push(Command c) {
		historyStack.push_front(c);
	}

	// Remove and return the last Command in the history stack
	// Output: a Command
	Command pop() {
		Command c = (Command)(historyStack.back());
		historyStack.pop_back();
		return c;
	}
};

/*-----------------------------------------------------------------------
		End of Classes
-----------------------------------------------------------------------*/

// Function prototypes for main program
void run_sh();
string getInput();
bool executeCommand(Command cmd);
void start(Command cmd);
int background(Command cmd);
bool moveToDir(Command cmd);
void freeArgs(char** args, int argsn);
void dalek(Command cmd);
int findChildPid(int pid);
void dalekall();
void repeat(Command cmd);

// Global variables
Command_Stack history; // History command stack
string currentdir;	   // The current working directory path
int status;			   // The status of the program - 1: Run, 0: End
vector<int> child_pids{};

int main() {

	status = 1; // Set status to run (1)

	// Get command history from file mysh_history.txt
	history.readFromFile();

	run_sh();

	// Print command history from file mysh_history.txt
	history.printToFile();
	
	return 0;
}

// Run the shell until byebye entered or a fatal error occurs
void run_sh() {
	// Get the current directory
	currentdir = get_current_dir_name();

	string inputString;

	// Run the shell while the status continues
	while (status) {
		// Get the input from the shell console and make it a Command
		inputString = getInput();
        Command cmd(inputString);
		
		// If the command is valid, execute it
        bool validCmd = executeCommand(cmd);
		
		// Add to history if command is not byebye or a history clear
        if (cmd.validCmd() && cmd.commandNum != byebye_sym && validCmd) {
			if (cmd.hasArgs() && (cmd.commandNum != history_sym || !cmd.argsIs("-c")))
				history.push(cmd);
			else if (!cmd.hasArgs())
				history.push(cmd);
		}
        else if (cmd.commandNum == byebye_sym) // status is now 0
            break;
        else								   // the command input is not recognized
			cout << "Invalid command: "<< cmd.cmdInput << endl;
    }
}


// Execute a given command from the shell based on its command number
// Input: Command cmd - The Command to execute
// Output: bool - True: Command was executed successfully
bool executeCommand(Command cmd) {
	pid_t pid;
	int c = cmd.commandNum;

	switch(c) {
		case movetodir_sym: // Move to given directory
			moveToDir(cmd);
			break;
		case whereami_sym:  // Print out current director
			currentdir = get_current_dir_name();
			cout << currentdir << endl;
			
			break;
		case history_sym:   // Print out the current history stack
			if (cmd.numTokens == 1)
				history.printHistory();
			else if (cmd.numTokens == 2 && cmd.argsIs("-c"))
				history.clearHistory();
			else
				return false;
			break;
		case byebye_sym:    // Set status to 0 and leave program
			status = 0;
			break;
		case replay_sym:    // Replay a given Command from the history stack
			executeCommand(history.findReplayNum(cmd));
			break;
		case start_sym:     // Start a program (with arguments), wait until it finishes
			start(cmd);
			break;
		case background_sym:// Start a program (with arguments), do not wait for it to finish
			background(cmd);
			break;
		case dalek_sym:		// End a process based on its pid
			dalek(cmd);
			break;
		case repeat_sym:	// Repeat a given command a given number of times
			repeat(cmd);
			break;
		case dalekall_sym:  // End all processes currently running in shell
			dalekall();
			break;
		default:
			return false;
	}

	return true;
}

// Repeat creating a background process a given number of times
void repeat(Command cmd) {
	const string BACKGROUND = "background";
	int nTimes = stoi(cmd.args[0]);
	
	string args = "";
	int numArgs = cmd.args.size();

	for (int i = 2; i < numArgs; i++)
		args += args[i] + " ";
	string cmdStr = BACKGROUND + " " + args;
	Command* rptCmd = new Command(cmdStr);
	int *pids = new int[nTimes];

	for (int i = 0; i < nTimes; i++) 
		pids[i] = background(*rptCmd);

}

// Terminate a process
// Input: Command cmd - The current Command
void dalek(Command cmd) {
	int pidToKill = stoi(cmd.args[0]);

	// Send signal to terminate process
	kill(pidToKill, SIGTERM);

	// Remove pid from vector of child processes running
	vector<int>::iterator it;
	int idx = findChildPid(pidToKill);
	int i = 0;
	for (it = child_pids.begin(); it != child_pids.end(); it++, i++) {
		if (i == idx) {
			child_pids.erase(it);
			break;
		}
	}

	if (idx < 0)
		cout << "Could not terminate PID: " << pidToKill;
}

// Terminate all child processes currently running
void dalekall() {
	int size = child_pids.size();
	int* pids = new int[size];
	for (int i = 0; i < size; i++) {
		kill(child_pids[i], SIGTERM);
		pids[i] = child_pids[i];
	}
	
	child_pids.clear();

	cout << "Exterminating " << size << " processes:";
	for (int i = 0; i < size; i++)
		cout << " " << pids[i];
	cout << endl;

	delete(pids);
}

// Find a process id in vector of child processes
// Output: index of the pid, or -1 if not found
int findChildPid(int pid) {
	int numChildren = child_pids.size();
	int i;
	for (i = 0; i < numChildren; i++) {
		if (child_pids[i] == pid)
			break;
	}

	return (i < numChildren) ? i : -1;
}

// Start given process in shell. To run program, precede program with ./
// Wait until it finishes to resume shell
// Input: Command cmd - The current Command
void start(Command cmd) {
	int numArgs = cmd.args.size();
  	char* args[numArgs];
	
	// Convert the Command arguments into a null terminated array of char pointers
	for (int i = 0; i < numArgs; i++)
		args[i] = strdup((cmd.args[i].c_str()));
	args[numArgs] = NULL;
	
	int stat;

	// Create a new process
   	pid_t c_pid = fork();

    if (c_pid == -1) {
        printf("  Failed forking child...\n");
    }
    else if (c_pid == 0) {
		// Execute the program and arguments
        if (execvp(args[0], args) < 0) {
            cout << "  Could not open: " << args[0] << endl;
			freeArgs(args, numArgs);
			exit(0); // exit the current process
        }
    }
    else {
		// Wait until the current child process is completed
        waitpid(c_pid, &stat, 0);
		freeArgs(args, numArgs);
    }
}


// Start given process in shell. To run program, precede program with ./
// Do not wait until it finishes to resume shell
// Input: Command cmd - The current Command
// Output: the pid of the child process
int background(Command cmd) {
	int numArgs = cmd.args.size();
  	char* args[numArgs];
	
	// Convert the Command arguments into a null terminated array of char pointers
	for (int i = 0; i < numArgs; i++)
		args[i] = strdup((cmd.args[i].c_str()));
	args[numArgs] = NULL;
	
	int stat;

	// Create a new process, add it to child processes, and print to console
   	pid_t c_pid = fork();
	if (c_pid > 0) {
		child_pids.push_back(c_pid);
		cout << "PID: " << c_pid << endl;
	}
	
	// Fork did not execute
    if (c_pid == -1) {
        printf("  Failed forking child...\n");
    }

	// Parent process executes program
    else if (c_pid == 0) {
		// Execute the program and arguments
        if (execvp(args[0], args) < 0) {
            cout << "  Could not open: " << args[0] << endl;
			freeArgs(args, numArgs);
			exit(0); // exit the current process
        }
    }

	// Child process action after execute
    else {
		// Wait until the current child process is completed
        waitpid(c_pid, &stat, WNOHANG);
		freeArgs(args, numArgs);
    }

	return c_pid;
}

// Move to the new specified directory
// Input: Command cmd - Command containing movetodir
// Output: bool - True: Successfully changed directores
bool moveToDir(Command cmd) {
	DIR* dir;
	struct dirent *entry;
	int count;
	char path[BUFFER_MAX];
	char* arg = strdup(cmd.args[0].c_str());
	if ((chdir(arg)) != 0) 
	{
		cout << "Directory " << arg << ": not found" << endl;
		return false;
	}
	else {
		return true;
	}
	if ((dir = opendir(arg)) == NULL) {
		cout << "Directory " << arg << ": not found" << endl;
		return false;
	}
	else {
		while((entry = readdir(dir)) != NULL) {
			if (entry->d_name[0] != '.') {
				strcpy(path, arg);
				strcat(path, "/");
				strcat(path, entry->d_name);
			}
		}
		
		cout << get_current_dir_name() << endl;

		closedir(dir);
	}

	return true;
}

// Delete tokens created to run process
// Input: char** args - static array of char pointers
//		  int argsn	  - number of arguments being deleted	  
void freeArgs(char** args, int argsn) {
	if (argsn == 0) return;
	else if (argsn == 1) delete(args[0]);
	else {
		for (int i = 0; i < argsn; i++) {
			if (args[i] != NULL)
				delete(args[i]);
		}
	}
}

// Get input from the shell console
// Output: string - the line read
string getInput() {
	// Use a buffer to prevent an overly large input
	char buffer[BUFFER_MAX];
	
	cout << "# ";
	cin.getline(buffer, BUFFER_MAX);

	return string(buffer);
}
