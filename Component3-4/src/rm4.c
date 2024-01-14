#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h> 
#include <errno.h> 

// Global variables
ino_t starting_filesystem_id = 0; // Filesystem ID of the starting directory
bool preserve_root = true; // Default behavior is to preserve root

// Function to check if a file or directory exists and is writable
int checkAccess(char file[]) {
    errno = 0; // Reset errno
    if (access(file, F_OK | W_OK) != 0) { // Check for existence and write permission
        perror(file); // Print error message
        return false;
    } else {
        return true;
    }
}

// Function to get filesystem ID of a directory
ino_t get_filesystem_id(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) { // Get file status
        perror("stat"); // Print error message
        return 0;
    }
    return statbuf.st_ino; // Return filesystem ID
}

// Function to remove a directory and its contents
int remove_directory(char *dirPath, bool oneFileSystem) {
    // Validate directory path
    if (dirPath == NULL) {
        fprintf(stderr, "Invalid directory path\n");
        return -1;
    }

    // Access check
    if (!checkAccess(dirPath)) {
        return -1;
    }

    // Check for preserve root
    if (preserve_root && strcmp(dirPath, "/") == 0) {
        fprintf(stderr, "Refusing to remove root directory\n");
        return -1;
    }

    DIR *dir = opendir(dirPath); // Open directory
    struct dirent *entry;
    char path[PATH_MAX];
    int r = 0;

    if (dir == NULL) {
        perror("opendir"); // Print error message
        return -1;
    }

    // Loop over directory entries
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(path, sizeof(path), "%s/%s", dirPath, entry->d_name); // Create full path
            ino_t current_filesystem_id = get_filesystem_id(path); // Get filesystem ID
            if (oneFileSystem && current_filesystem_id != starting_filesystem_id) {
                continue;
            }

            // Check if entry is a directory or a file
            if (entry->d_type == DT_DIR) {
                r = remove_directory(path, oneFileSystem); // Recursive call for directory
            } else {
                r = unlink(path); // Remove file
            }
            if (r != 0) {
                break;
            }
        }
    }
    closedir(dir); // Close directory

    // Remove directory if no errors occurred
    if (r == 0) {
        if (rmdir(dirPath) != 0) {
            perror("rmdir"); // Print error message
            return -1;
        }
    }
    return r;
}

// Function to remove an empty directory
int remove_empty_directory(char *dirPath) {
    // Validate directory path
    if (dirPath == NULL) {
        perror("Invalid directory path");
        return -1;
    }

    // Access check
    if (!checkAccess(dirPath)) {
        // perror already called in checkAccess
        return -1;
    }

    // Check for preserve root
    if (preserve_root && strcmp(dirPath, "/") == 0) {
        fprintf(stderr, "Refusing to remove root directory\n");
        return -1;
    }

    DIR *dir = opendir(dirPath); // Open directory
    if (dir == NULL) {
        perror("opendir failed"); // Print error message
        return -1;
    }

    struct dirent *entry;
    int isEmpty = 1;

    // Check if directory is empty
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            isEmpty = 0;
            break;
        }
    }

    if (closedir(dir) != 0) {
        perror("closedir failed"); // Print error message
        return -1;
    }

    // Remove directory if it is empty
    if (isEmpty) {
        errno = 0;  // Reset errno before the rmdir call
        if (rmdir(dirPath) != 0) {
            perror("rmdir failed"); // Print error message
            return -1;
        }
    } else {
        fprintf(stderr, "%s is not empty\n", dirPath);
        // Return a specific error code or do not call perror here
        return -1;
    }
    return 0;
}

// Function to ask user for confirmation before removing a file or directory
int interactive(char file[]){
    char interactiveYN[1];
    printf("Are you sure you want to remove '%s'? ", file); // Ask for confirmation

    gets(interactiveYN); // Get user input

    // Check user input
    if (interactiveYN[0] == 'y' || interactiveYN[0] == 'Y') {
        return true;
    } else {
        return false;
    }
}

// Function to print a message indicating whether a file or directory was successfully removed
void verbose(char file[], int removalStatus) {
    if (removalStatus == 0) {
        printf("Deleted %s successfully\n", file); // Print success message
    } else {
        printf("Error occurred while deleting %s\n", file); // Print error message
    }
}

// Function to handle errors that may occur when removing a file or directory
int unLinkErrorHandling(int removalStatus, char file[]) {
    if (removalStatus != 0) {
        fprintf(stderr, "%s: ", file); 
        perror(""); // perror will now correctly report the last error
        return false;
    } else {
        return true;
    }
}



// Main function with command line arguments
int main(int argc, char *argv[]) {

    // Array of available command line options
    char *availableOptions[] = {"-f","--force","-r", "-R", "--recursive","-d","--dir", "-i", "-I","--interactive=never", "-v", "--verbose","--one-file-system",\
    "--no-preserve-root","--preserve-root", "--help", "--version","--interactive=once","--interactive=always"};
    
    // Calculate the number of available options
    int numOptions = sizeof(availableOptions) / sizeof(availableOptions[0]);
    
    // Array to store which options are used
    bool options[numOptions];

    // Initialize options array to false
    memset(options, false, sizeof(options));
    
    // Declare and initialize option variables
    bool verboseMode = false, interactiveMode = false, moreThanThreeInteractiveMode = false,
    removeDirectories = false, removeEmptyDirectories = false, forceMode = false,
    interactiveWhenNever = false, interactiveWhenOnce = false, interactiveWhenAlways = false,
    oneFileSystemMode = false, noPreserveRootMode = false, preserveRootMode = false,
    help = false, version = false;

    // Index to keep track of where the file names start in the command line arguments
    int fileStartIndex = 1;
    char interactiveYN;

    // Loop through command line arguments
    for (int i = 1; i < argc; i++) {
        bool isOption = false;
        // Loop through available options
        for (int j = 0; j < numOptions; j++) {
            // If the command line argument matches an available option
            if (strcmp(argv[i], availableOptions[j]) == 0) {
                // Mark the option as used
                options[j] = true;
                isOption = true;

                // Set the corresponding option variable to true
                if (j == 18) interactiveWhenAlways = true;
                if (j== 17) interactiveWhenOnce = true;
                if (j == 16) version = true;
                if (j == 15 ) help = true;
                if (j == 14) preserveRootMode = true;
                if (j == 13) noPreserveRootMode = true;
                if (j == 12) oneFileSystemMode = true;
                if (j == 10 || j == 11) verboseMode = true; 
                if (j == 9) interactiveWhenNever = true;
                if (j == 8) moreThanThreeInteractiveMode = true;
                if (j == 7 ) interactiveMode = true;
                if (j == 5 || j == 6 ) removeEmptyDirectories = true;
                if (j == 2 || j == 3 || j == 4) removeDirectories = true; 
                if (j == 0 || j == 1) forceMode = true; 

                // Increment the file start index
                fileStartIndex++;
                break;
            }
        }
        // If the command line argument is not an option, break the loop
        if (!isOption) break;
    }
    // Check if both removeDirectories and removeEmptyDirectories flags are false
    if (!removeDirectories && !removeEmptyDirectories){

        // If version flag is true, print the version details
        if(version){
            // Print the version details
            printf("Custom rm - Version 4\n");
            printf("Component 3 of Operating Systems Assignment\n");
            printf("Developed by Will Griffin\n\n");
            printf("This version of rm is a custom implementation created as part of an academic assignment. ");
            printf("Its features and behaviors are identical to the standard rm command found in Unix and GNU/Linux systems.\n\n");
            printf("This software is intended for educational purposes and mirrors the functionality of traditional Unix/Linux rm tools.\n");
            printf("Copyright notice: This software is used for educational purposes and is not intended for commercial use.\n");
            // Exit the program
            return 0;
        }

        // If help flag is true, print the help details
        if(help){
            // Print the usage details
            printf("Usage: rm [OPTION]... [FILE]...\n");
            printf("Remove (unlink) the FILE(s).\n\n");
            printf("  -f, --force           ignore nonexistent files and arguments, never prompt\n");
            printf("  -i                    prompt before every removal\n");
            printf("  -I                    prompt once before removing more than three files, or\n");
            printf("                          when removing recursively; less intrusive than -i,\n");
            printf("                          while still giving protection against most mistakes\n");
            printf("      --interactive[=WHEN]  prompt according to WHEN: never, once (-I), or\n");
            printf("                          always (-i); without WHEN, prompt always\n");
            printf("      --one-file-system  when removing a hierarchy recursively, skip any\n");
            printf("                          directory that is on a file system different from\n");
            printf("                          that of the corresponding command line argument\n");
            printf("      --no-preserve-root  do not treat '/' specially\n");
            printf("      --preserve-root[=all]  do not remove '/' (default);\n");
            printf("                              with 'all', reject any command line argument\n");
            printf("                              on a separate device from its parent\n");
            printf("  -r, -R, --recursive   remove directories and their contents recursively\n");
            printf("  -d, --dir             remove empty directories\n");
            printf("  -v, --verbose         explain what is being done\n");
            printf("      --help     display this help and exit\n");
            printf("      --version  output version information and exit\n\n");
            printf("By default, rm does not remove directories.  Use the --recursive (-r or -R)\n");
            printf("option to remove each listed directory, too, along with all of its contents.\n");

            // Exit the program
            return 0;
        }
        
        // Check if forceMode is enabled
        if (forceMode) {
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If interactiveMode is enabled and user doesn't confirm deletion, skip this file
                if(interactiveMode && !interactive(argv[i])){
                    continue;
                } 
                // If verboseMode is enabled, print the file name and the result of the unlink operation
                if (verboseMode) {
                    verbose(argv[i], unlink(argv[i]));
                } else {
                    // If verboseMode is not enabled, just perform the unlink operation
                    unlink(argv[i]);
                }
            }
            // Return 0 to indicate successful execution
            return 0;
        }

        // Check if interactiveMode is enabled
        if(interactiveMode){
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If user confirms deletion for the current file
                if(interactive(argv[i])){
                    // If verboseMode is enabled
                    if(verboseMode){
                        // If the file is accessible, delete it and print the file name and the result of the unlink operation
                        if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));
                    }else{
                        // If verboseMode is not enabled, delete the file and handle any error that might occur during the unlink operation
                        if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                    }
                }
            }
            // Return 0 to indicate successful execution
            return 0; 
            }
        // Check if verboseMode is enabled and all interactive modes are disabled
        if(verboseMode && !interactiveMode && !moreThanThreeInteractiveMode && !interactiveWhenNever && !interactiveWhenOnce && !interactiveWhenAlways){
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If the file is accessible, delete it and print the file name and the result of the unlink operation
                if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));
            }
            // Return 0 to indicate successful execution
            return 0;
        }
        // Check if moreThanThreeInteractiveMode is enabled
        if (moreThanThreeInteractiveMode){
            // Initialize askedOnce flag to false
            bool askedOnce = false;
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If the number of files is more than 3
                if((argc-fileStartIndex) > 3){
                    // If user hasn't been asked for confirmation yet
                    if(!askedOnce){
                        // Ask user for confirmation
                        printf("remove %d files?\n", argc-fileStartIndex);
                        char response = getchar();
                        // If user confirms deletion, set askedOnce flag to true
                        if(response == 'y' || response == 'Y'){
                            askedOnce = true;
                        }
                    }
                    // If user has confirmed deletion
                    if(askedOnce){
                        // If verboseMode is enabled, delete the file and print the file name and the result of the unlink operation
                        if(verboseMode){
                            if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));
                        }else{
                            // If verboseMode is not enabled, delete the file and handle any error that might occur during the unlink operation
                            if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                        }
                    }
                }else{
                    // If the number of files is 3 or less, delete the file and handle verboseMode and errors the same way as above
                    if(verboseMode){
                        if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));    
                    }else{
                        if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                    }
                }
            }
            // Return 0 to indicate successful execution
            return 0;
        }

        // Check if interactiveWhenNever mode is enabled
        if (interactiveWhenNever) {
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If verboseMode is enabled
                if (verboseMode) {
                    // If the file is accessible, delete it and print the file name and the result of the unlink operation
                    if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));
                } else {
                    // If verboseMode is not enabled, delete the file and handle any error that might occur during the unlink operation
                    if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                }
            }
            // Return 0 to indicate successful execution
            return 0;
        }

    
        // Check if interactiveWhenOnce mode is enabled
        if (interactiveWhenOnce) {
            // Initialize askedOnce flag to false
            bool askedOnce = false;
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If the number of files is more than 3
                if((argc-fileStartIndex) > 3){
                    // If user hasn't been asked for confirmation yet
                    if(!askedOnce){
                        // Ask user for confirmation
                        printf("remove %d files?\n", argc-fileStartIndex);
                        char response = getchar();
                        // If user confirms deletion, set askedOnce flag to true
                        if(response == 'y' || response == 'Y'){
                            askedOnce = true;
                        }
                    }
                    // If user has confirmed deletion
                    if(askedOnce){
                        // If verboseMode is enabled, delete the file and print the file name and the result of the unlink operation
                        if(verboseMode){
                            if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));   
                        }else{
                            // If verboseMode is not enabled, delete the file and handle any error that might occur during the unlink operation
                            if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                        }
                    }
                }else{
                    // If the number of files is 3 or less, delete the file and handle verboseMode and errors the same way as above
                    if(verboseMode){
                        if (checkAccess(argv[i])) verbose(argv[i],unLinkErrorHandling(unlink(argv[i]),argv[i]));        
                    }else{
                        if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                    }
                }
            }
            // Return 0 to indicate successful execution
            return 0;
        }
        // Check if interactiveWhenAlways mode is enabled
        if (interactiveWhenAlways) {
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If user confirms deletion for the current file
                if (interactive(argv[i])) {
                    // If verboseMode is enabled
                    if (verboseMode) {
                        // If the file is accessible, delete it and print the file name and the result of the unlink operation
                        if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));       
                    } else {
                        // If verboseMode is not enabled, delete the file and handle any error that might occur during the unlink operation
                        if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                    }
                }
            }
            // Return 0 to indicate successful execution
            return 0;
        }
    } else {
        // Check if noPreserveRootMode is enabled
        if (noPreserveRootMode) {
            // If enabled, set preserve_root to false
            preserve_root = false;
        }

        // Check if oneFileSystemMode is enabled
        if (oneFileSystemMode) {
            // If enabled, get the filesystem id of the first non-option argument (assumed to be the target path)
            starting_filesystem_id = get_filesystem_id(argv[fileStartIndex]);
        }

        // Check if removeDirectories is enabled
        if (removeDirectories) {
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If interactiveMode is enabled and user doesn't confirm deletion, skip this file
                if (interactiveMode && !interactive(argv[i])) {
                    continue;
                }

                // Remove the directory and store the status of the operation
                int removalStatus = remove_directory(argv[i], oneFileSystemMode); // Pass oneFileSystemMode
                // If verboseMode is enabled, print the file name and the result of the removal operation
                if (verboseMode) {
                    verbose(argv[i], removalStatus);
                } else {
                    // If verboseMode is not enabled, handle any error that might occur during the removal operation
                    unLinkErrorHandling(removalStatus, argv[i]);
                }
            }
        } 

        // Check if removeEmptyDirectories is enabled
        if (removeEmptyDirectories) {
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If interactiveMode is enabled and user doesn't confirm deletion, skip this file
                if (interactiveMode && !interactive(argv[i])) {
                    continue;
                }

                // Remove the empty directory and store the status of the operation
                int removalStatus = remove_empty_directory(argv[i]);
                // If verboseMode is enabled, print the file name and the result of the removal operation
                if (verboseMode) {
                    verbose(argv[i], removalStatus);
                } else {
                    // If verboseMode is not enabled, handle any error that might occur during the removal operation
                    unLinkErrorHandling(removalStatus, argv[i]);
                }
            }
        }

    }
        // Check if none of the modes are enabled
        if (!version && !help && !preserveRootMode && !noPreserveRootMode && !oneFileSystemMode && !verboseMode && !interactiveWhenNever && !moreThanThreeInteractiveMode && !interactiveMode && !removeEmptyDirectories && !removeDirectories && !forceMode) {
            // Loop through all the files starting from fileStartIndex
            for (int i = fileStartIndex; i < argc; i++) {
                // If the file is accessible, delete it and handle any error that might occur during the unlink operation
                if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]), argv[i]);
            }
            // Return 0 to indicate successful execution
            return 0;
        }       

        // If none of the above conditions are met, return 0 to indicate successful execution
        return 0;
}