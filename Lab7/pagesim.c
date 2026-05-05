/*
 * pagesim.c - Page Replacement Policy Simulator
 * CprE 308/3080 - Lab 7: Memory Management
 *
 * Usage: ./pagesim <num_frames> <policy> <ref_string_or_file>
 *   policy: fifo, lru, opt
 *
 * Example:
 *   ./pagesim 3 fifo "7 0 1 2 0 3 0 4 2 3 0 3 2"
 *   ./pagesim 4 lru refstring.txt
 *
 * YOUR TASK: Implement simulate_fifo(), simulate_lru(), simulate_opt().
 *
 * Each function receives:
 *   - ref_string[]: array of page numbers to access, in order
 *   - ref_len:      length of the reference string
 *   - num_frames:   number of physical frames available
 *
 * Each function must:
 *   - Simulate the policy, printing each step using print_step()
 *   - Return the total number of page faults
 *
 * Frames start empty (use -1 to represent empty).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REF_LENGTH 1000
#define MAX_FRAMES     100

/*
 * Print one step of the simulation. Call this once per reference.
 *   step:     1-indexed step number
 *   page:     the page being referenced
 *   frames:   array of current frame contents (-1 = empty)
 *   n:        number of frames
 *   is_fault: 1 if page fault, 0 if hit
 */
void print_step(int step, int page, int frames[], int n, int is_fault) {
    printf("Step %3d: Page %2d -> [", step, page);
    for (int i = 0; i < n; i++) {
        if (frames[i] == -1) printf("  -");
        else printf(" %2d", frames[i]);
        if (i < n - 1) printf(",");
    }
    printf(" ] %s\n", is_fault ? "FAULT" : "HIT");
}

/*
 * FIFO: On a page fault, evict the page that arrived in memory earliest.
 */
int simulate_fifo(int ref_string[], int ref_len, int num_frames) {
    int frames[MAX_FRAMES];
    int faults = 0;
    int next_replace = 0;

    for (int i = 0; i < num_frames; i++) frames[i] = -1;

    for (int i = 0; i < ref_len; i++) {
        int page = ref_string[i];
        int hit = 0;

        for (int j = 0; j < num_frames; j++) {
            if (frames[j] == page) {
                hit = 1;
                break;
            }
        }

        int is_fault = 0;
        if (!hit) {
            is_fault = 1;
            faults++;
            frames[next_replace] = page;
            next_replace = (next_replace + 1) % num_frames;
        }

        print_step(i + 1, page, frames, num_frames, is_fault);
    }
    return faults;
}

/*
 * LRU: On a page fault, evict the page whose most recent access
 * is the farthest in the past.
 */
int simulate_lru(int ref_string[], int ref_len, int num_frames) {
    int frames[MAX_FRAMES];
    int last_used[MAX_FRAMES];
    int faults = 0;

    for (int i = 0; i < num_frames; i++) {
        frames[i] = -1;
        last_used[i] = -1;
    }

    for (int i = 0; i < ref_len; i++) {
        int page = ref_string[i];
        int hit = -1;

        for (int j = 0; j < num_frames; j++) {
            if (frames[j] == page) {
                hit = j;
                break;
            }
        }

        int is_fault = 0;
        if (hit != -1) {
            last_used[hit] = i;
        } else {
            is_fault = 1;
            faults++;
            
            int lru_idx = 0;
            for (int j = 1; j < num_frames; j++) {
                if (frames[j] == -1) {
                    lru_idx = j;
                    break;
                }
                if (last_used[j] < last_used[lru_idx]) {
                    lru_idx = j;
                }
            }
            frames[lru_idx] = page;
            last_used[lru_idx] = i;
        }

        print_step(i + 1, page, frames, num_frames, is_fault);
    }
    return faults;
}

/*
 * OPT (Belady's): On a page fault, evict the page that will not be
 * used for the longest time in the future.
 */
int simulate_opt(int ref_string[], int ref_len, int num_frames) {
    int frames[MAX_FRAMES];
    int faults = 0;

    for (int i = 0; i < num_frames; i++) frames[i] = -1;

    for (int i = 0; i < ref_len; i++) {
        int page = ref_string[i];
        int hit = 0;

        for (int j = 0; j < num_frames; j++) {
            if (frames[j] == page) {
                hit = 1;
                break;
            }
        }

        int is_fault = 0;
        if (!hit) {
            is_fault = 1;
            faults++;

            int replace_idx = -1;
            for (int j = 0; j < num_frames; j++) {
                if (frames[j] == -1) {
                    replace_idx = j;
                    break;
                }
            }

            if (replace_idx == -1) {
                int farthest = -1;
                for (int j = 0; j < num_frames; j++) {
                    int next_use = 1000000; // Infinity
                    for (int k = i + 1; k < ref_len; k++) {
                        if (ref_string[k] == frames[j]) {
                            next_use = k;
                            break;
                        }
                    }
                    if (next_use > farthest) {
                        farthest = next_use;
                        replace_idx = j;
                    }
                }
            }
            frames[replace_idx] = page;
        }

        print_step(i + 1, page, frames, num_frames, is_fault);
    }
    return faults;
}

/* ================================================================
 * Infrastructure below -- you should not need to modify this.
 * ================================================================ */

int parse_ref_string(const char *str, int ref_string[], int max_len) {
    int count = 0;
    char *copy = strdup(str);
    char *token = strtok(copy, " ,\t\n");
    while (token != NULL && count < max_len) {
        ref_string[count++] = atoi(token);
        token = strtok(NULL, " ,\t\n");
    }
    free(copy);
    return count;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <num_frames> <policy> <ref_string_or_file>\n", argv[0]);
        fprintf(stderr, "  policy: fifo, lru, opt\n");
        fprintf(stderr, "\nExamples:\n");
        fprintf(stderr, "  %s 3 fifo \"7 0 1 2 0 3 0 4 2 3 0 3 2\"\n", argv[0]);
        fprintf(stderr, "  %s 4 lru refstring.txt\n", argv[0]);
        return 1;
    }

    int num_frames = atoi(argv[1]);
    if (num_frames <= 0 || num_frames > MAX_FRAMES) {
        fprintf(stderr, "Error: num_frames must be between 1 and %d.\n", MAX_FRAMES);
        return 1;
    }

    char *policy = argv[2];
    int ref_string[MAX_REF_LENGTH];
    int ref_len;

    FILE *f = fopen(argv[3], "r");
    if (f) {
        char buf[MAX_REF_LENGTH * 4];
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = '\0';
        fclose(f);
        ref_len = parse_ref_string(buf, ref_string, MAX_REF_LENGTH);
    } else {
        ref_len = parse_ref_string(argv[3], ref_string, MAX_REF_LENGTH);
    }

    if (ref_len == 0) {
        fprintf(stderr, "Error: empty reference string.\n");
        return 1;
    }

    printf("=== Page Replacement Simulation ===\n");
    printf("Policy: %s | Frames: %d | References: %d\n", policy, num_frames, ref_len);
    printf("Reference string: ");
    for (int i = 0; i < ref_len; i++) printf("%d ", ref_string[i]);
    printf("\n\n");

    int faults;

    if (strcmp(policy, "fifo") == 0) {
        faults = simulate_fifo(ref_string, ref_len, num_frames);
    } else if (strcmp(policy, "lru") == 0) {
        faults = simulate_lru(ref_string, ref_len, num_frames);
    } else if (strcmp(policy, "opt") == 0) {
        faults = simulate_opt(ref_string, ref_len, num_frames);
    } else {
        fprintf(stderr, "Error: unknown policy '%s'. Use fifo, lru, or opt.\n", policy);
        return 1;
    }

    printf("\nTotal page faults: %d / %d references\n", faults, ref_len);
    printf("Hit rate: %.1f%%\n", 100.0 * (ref_len - faults) / ref_len);

    return 0;
}
