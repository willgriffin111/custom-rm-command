#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h> 
#include <errno.h> 

ino_t starting_filesystem_id = 0;
bool preserve_root = true; // Default behavior is to preserve root

int checkAccess(char file[]) {
    errno = 0; // Reset errno before calling access
    if (access(file, F_OK | W_OK) != 0) {
        perror(file); // Will now correctly report errors related to access
        return false;
    } else {
        return true;
    }
}


// Function to get filesystem ID of a directory
ino_t get_filesystem_id(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        perror("stat");
        return 0;
    }
    return statbuf.st_ino;
}

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

    DIR *dir = opendir(dirPath);
    struct dirent *entry;
    char path[PATH_MAX];
    int r = 0;

    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(path, sizeof(path), "%s/%s", dirPath, entry->d_name);
            ino_t current_filesystem_id = get_filesystem_id(path);
            if (oneFileSystem && current_filesystem_id != starting_filesystem_id) {
                continue;
            }

            if (entry->d_type == DT_DIR) {
                r = remove_directory(path, oneFileSystem);
            } else {
                r = unlink(path);
            }
            if (r != 0) {
                break;
            }
        }
    }
    closedir(dir);

    if (r == 0) {
        if (rmdir(dirPath) != 0) {
            perror("rmdir");
            return -1;
        }
    }
    return r;
}


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

    DIR *dir = opendir(dirPath);
    if (dir == NULL) {
        perror("opendir failed");
        return -1;
    }

    struct dirent *entry;
    int isEmpty = 1;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            isEmpty = 0;
            break;
        }
    }

    if (closedir(dir) != 0) {
        perror("closedir failed");
        return -1;
    }

    if (isEmpty) {
        errno = 0;  // Reset errno before the rmdir call
        if (rmdir(dirPath) != 0) {
            perror("rmdir failed");
            return -1;
        }
    } else {
        fprintf(stderr, "%s is not empty\n", dirPath);
        // Return a specific error code or do not call perror here
        return -1;
    }
    return 0;
}


int interactive(char file[]){

    char interactiveYN[1];
    printf("Are you sure you want to remove '%s'? ", file);

    gets(interactiveYN);

    // printf("\nEntered: %s",interactiveYN);

    if (interactiveYN[0] == 'y' || interactiveYN[0] == 'Y') {
        return true;
    } else {
        return false;
    }
}


void verbose(char file[], int removalStatus) {
    // printf("\nVERBOSE");
    if (removalStatus == 0) {
        printf("Deleted %s successfully\n", file);

    } else {
        // Only print error if there is an error
        printf("Error occurred while deleting %s\n", file);
    }
}


int unLinkErrorHandling(int removalStatus, char file[]) {
    if (removalStatus != 0) {
        fprintf(stderr, "%s: ", file); 
        perror(""); // perror will now correctly report the last error
        return false;
    } else {
        return true;
    }
}



int main(int argc, char *argv[]) {

    char *availableOptions[] = {"-f","--force","-r", "-R", "--recursive","-d","--dir", "-i", "-I","--interactive=never", "-v", "--verbose","--one-file-system",\
    "--no-preserve-root","--preserve-root", "--help", "--version","--interactive=once","--interactive=always"};
    int numOptions = sizeof(availableOptions) / sizeof(availableOptions[0]);
    bool options[numOptions];

    memset(options, false, sizeof(options));
    bool verboseMode = false, interactiveMode = false, moreThanThreeInteractiveMode = false,
    removeDirectories = false, removeEmptyDirectories = false, forceMode = false,
    interactiveWhenNever = false, interactiveWhenOnce = false, interactiveWhenAlways = false,
    oneFileSystemMode = false, noPreserveRootMode = false, preserveRootMode = false,
    help = false, version = false;

    int fileStartIndex = 1;
    char interactiveYN;

    for (int i = 1; i < argc; i++) {
        bool isOption = false;
        for (int j = 0; j < numOptions; j++) {
            if (strcmp(argv[i], availableOptions[j]) == 0) {
                options[j] = true;
                isOption = true;

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

                fileStartIndex++;
                break;
            }
        }
        if (!isOption) break;
    }
    if (!removeDirectories && !removeEmptyDirectories){

        if(version){
            printf("Custom rm - Version 4\n");
            printf("Component 3 of Operating Systems Assignment\n");
            printf("Developed by Will Griffin\n\n");
            printf("This version of rm is a custom implementation created as part of an academic assignment. ");
            printf("Its features and behaviors are identical to the standard rm command found in Unix and GNU/Linux systems.\n\n");
            printf("This software is intended for educational purposes and mirrors the functionality of traditional Unix/Linux rm tools.\n");
            printf("Copyright notice: This software is used for educational purposes and is not intended for commercial use.\n");
            return 0;
        }

        if(help){
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

            return 0;
        }
        
        if (forceMode) {
            for (int i = fileStartIndex; i < argc; i++) {
                if(interactiveMode && !interactive(argv[i])){
                    continue;
                } 
                if (verboseMode) {
                    verbose(argv[i], unlink(argv[i]));
                } else {
                    unlink(argv[i]);
                }
            
            }
            return 0;
        }

        if(interactiveMode){
            for (int i = fileStartIndex; i < argc; i++) {
                if(interactive(argv[i])){
                    if(verboseMode){
                        if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));
   
                    }else{
                        if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                    }
                }
            }
            return 0; 
        }
        if(verboseMode && !interactiveMode && !moreThanThreeInteractiveMode && !interactiveWhenNever && !interactiveWhenOnce && !interactiveWhenAlways){
            for (int i = fileStartIndex; i < argc; i++) {
                // verbose(argv[i],unlink(argv[i]));

                if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));

                // if (checkAccess(argv[i])) verbose(argv[i],unLinkErrorHandling(unlink(argv[i]),argv[i]));

            }
            return 0;
        }
    if (moreThanThreeInteractiveMode){
        bool askedOnce = false;
        for (int i = fileStartIndex; i < argc; i++) {
            if((argc-fileStartIndex) > 3){
                if(!askedOnce){
                    printf("remove %d files?\n", argc-fileStartIndex);
                    char response = getchar();
                    if(response == 'y' || response == 'Y'){
                        askedOnce = true;
                    }
                }
                if(askedOnce){
                    if(verboseMode){
                        // verbose(argv[i],unlink(argv[i]));  
                        if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));
                    }else{
                        // unLinkErrorHandling(unlink(argv[i]),argv[i]); 
                         if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                    }
                }
            }else{
                if(verboseMode){
                    // verbose(argv[i],unlink(argv[i]));    
                    if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));    
                }else{
                    // unLinkErrorHandling(unlink(argv[i]),argv[i]); 
                    if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                }
            }
        }
        return 0;
    }

    if (interactiveWhenNever) {
        for (int i = fileStartIndex; i < argc; i++) {
            if (verboseMode) {
                // verbose(argv[i], unlink(argv[i]));
                if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));
            } else {
                // unLinkErrorHandling(unlink(argv[i]), argv[i]);
                 if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
            }
        }
        return 0;
    }

   
    if (interactiveWhenOnce) {
        bool askedOnce = false;
        for (int i = fileStartIndex; i < argc; i++) {
            if((argc-fileStartIndex) > 3){
                if(!askedOnce){
                    printf("remove %d files?\n", argc-fileStartIndex);
                    char response = getchar();
                    if(response == 'y' || response == 'Y'){
                        askedOnce = true;
                    }
                }
                if(askedOnce){
                    if(verboseMode){
                        // verbose(argv[i],unlink(argv[i]));  
                        if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));   
                    }else{
                        // unLinkErrorHandling(unlink(argv[i]),argv[i]); 
                        if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                    }
                }
            }else{
                if(verboseMode){
                    // verbose(argv[i],unlink(argv[i]));   
                    if (checkAccess(argv[i])) verbose(argv[i],unLinkErrorHandling(unlink(argv[i]),argv[i]));        
                }else{
                    // unLinkErrorHandling(unlink(argv[i]),argv[i]); 
                     if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                }
            }
        }
        return 0;
    }
    if (interactiveWhenAlways) {
        for (int i = fileStartIndex; i < argc; i++) {
            if (interactive(argv[i])) {
                if (verboseMode) {
                    // verbose(argv[i], unlink(argv[i]));
                    if (checkAccess(argv[i])) verbose(argv[i],unlink(argv[i]));       
                } else {
                    // unLinkErrorHandling(unlink(argv[i]), argv[i]);
                     if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]),argv[i]);
                }
            }
        }
        return 0;
    }
} else {
    // Global variables for root preservation and one file system mode
    if (noPreserveRootMode) {
        preserve_root = false;
    }
    if (oneFileSystemMode) {
        // Assuming the first non-option argument is the target path
        starting_filesystem_id = get_filesystem_id(argv[fileStartIndex]);
    }

    if (removeDirectories) {
        for (int i = fileStartIndex; i < argc; i++) {
            if (interactiveMode && !interactive(argv[i])) {
                continue;
            }

            int removalStatus = remove_directory(argv[i], oneFileSystemMode); // Pass oneFileSystemMode
            if (verboseMode) {
                verbose(argv[i], removalStatus);
            } else {
                unLinkErrorHandling(removalStatus, argv[i]);
            }
        }
    } 

   if (removeEmptyDirectories) {
    for (int i = fileStartIndex; i < argc; i++) {
        if (interactiveMode && !interactive(argv[i])) {
            continue;
        }

        int removalStatus = remove_empty_directory(argv[i]);
        if (verboseMode) {
            verbose(argv[i], removalStatus);
        } 
        // else {
            // Check if removalStatus indicates an error and handle it appropriately
        //     if (removalStatus != 0) {
        //         fprintf(stderr, "Error removing %s\n", argv[i]);
        //     }
        // }
    }
}

}
    if (!version && !help && !preserveRootMode && !noPreserveRootMode && !oneFileSystemMode && !verboseMode && !interactiveWhenNever && !moreThanThreeInteractiveMode && !interactiveMode && !removeEmptyDirectories && !removeDirectories && !forceMode) {
  
        for (int i = fileStartIndex; i < argc; i++) {

            if (checkAccess(argv[i])) unLinkErrorHandling(unlink(argv[i]), argv[i]);

        }
        return 0;
    }       

    return 0;
}