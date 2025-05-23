/**
 * git_integration.c
 * Implementation of Git repository detection and information
 */

#include "git_integration.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Initialize Git integration
 */
void init_git_integration(void) {
    // No initialization needed for Linux
}

/**
 * Check if the current directory is in a Git repository and get branch info
 */
int get_git_branch(char *branch_name, size_t buffer_size, int *is_dirty) {
    char git_dir[PATH_MAX] = "";
    char cmd[PATH_MAX] = "";
    FILE *fp;
    int status = 0;

    // Initialize output parameters
    if (branch_name && buffer_size > 0) {
        branch_name[0] = '\0';
    }
    if (is_dirty) {
        *is_dirty = 0;
    }

    // First check if .git directory exists (faster than running git command)
    strcpy(git_dir, ".git");
    struct stat st;
    if (stat(git_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        // Try to find .git in parent directories
        // (Note: this is more complex, simplified here)
        return 0;
    }

    // Get current branch
    if (branch_name && buffer_size > 0) {
        strcpy(cmd, "git rev-parse --abbrev-ref HEAD 2>/dev/null");
        fp = popen(cmd, "r");
        if (fp) {
            if (fgets(branch_name, buffer_size, fp) != NULL) {
                // Remove trailing newline
                char *newline = strchr(branch_name, '\n');
                if (newline) {
                    *newline = '\0';
                }
                status = 1;
            }
            pclose(fp);
        }
    }

    // Check if working directory is dirty
    if (is_dirty && status) {
        strcpy(cmd, "git status --porcelain 2>/dev/null");
        fp = popen(cmd, "r");
        if (fp) {
            char ch;
            if ((ch = fgetc(fp)) != EOF) {
                *is_dirty = 1;
            }
            pclose(fp);
        }
    }

    return status;
}

/**
 * Get the name of the Git repository
 */
int get_git_repo_name(char *repo_name, size_t buffer_size) {
    if (!repo_name || buffer_size == 0) {
        return 0;
    }

    repo_name[0] = '\0';

    char cmd[PATH_MAX] = "git rev-parse --show-toplevel 2>/dev/null";
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }

    char path[PATH_MAX];
    if (fgets(path, sizeof(path), fp) == NULL) {
        pclose(fp);
        return 0;
    }
    pclose(fp);

    // Remove trailing newline
    char *newline = strchr(path, '\n');
    if (newline) {
        *newline = '\0';
    }

    // Extract the directory name from the full path
    char *last_slash = strrchr(path, '/');
    if (last_slash) {
        strncpy(repo_name, last_slash + 1, buffer_size - 1);
        repo_name[buffer_size - 1] = '\0';
        return 1;
    }

    return 0;
}

/**
 * Get Git status for the current repository
 */
char *get_git_status(void) {
    char branch_name[100] = "";
    int is_dirty = 0;
    char repo_name[100] = "";
    
    // Check if we're in a Git repo and get branch info
    if (!get_git_branch(branch_name, sizeof(branch_name), &is_dirty)) {
        return NULL; // Not in a Git repo
    }
    
    // Get repo name
    get_git_repo_name(repo_name, sizeof(repo_name));
    
    // Format status string
    char *status = malloc(256);
    if (!status) {
        return NULL;
    }
    
    if (strlen(repo_name) > 0) {
        sprintf(status, "%s%s (%s%s%s)", 
                repo_name, 
                branch_name[0] ? ":" : "",
                branch_name, 
                is_dirty ? " *" : "",
                "");
    } else {
        sprintf(status, "%s%s", 
                branch_name, 
                is_dirty ? " *" : "");
    }
    
    return status;
}