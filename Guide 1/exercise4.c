#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>

typedef enum {
    RED,
    GREEN,
    YELLOW
} Light;

/** 
 * Verifies if some keyboard button was pressed in real time 
 * without blocking the main flow of the program
 * @returns > 0: There's available data in some descriptor monitored
 *            0: There's no data and time expired
 *          < 0: Error happened
*/
int button_pressed() {
    // Waiting time (timeout) for select()
    // 0 seconds and 0 microseconds means that select won't wait and will return inmediately
    struct timeval tv = {0L, 0L};

    /**
     * Set of archive descriptors, this monitors if there's available data in
     * standart input (stdin)
     * */
    fd_set fds;

    // Initializes set fds of descriptors
    FD_ZERO(&fds);

    // Adds archive descriptor stdin to fds, this will monitor if there's available data in stdin
    FD_SET(STDIN_FILENO, &fds);

    /**
     * @param STDIN_FILENO total number of descriptors to monitor
     * @param fds set of descriptors to monitor for reading
     * @param NULL there's no monitors for writing
     * @param NULL there's no monitors for exceptions
     * @param tv timeout. In this case is 0, select won't block
     */
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

void traffic_light();

int main(int argc, char const *argv[]) {
    traffic_light();
    return 0;
}

void traffic_light() {
    Light state = GREEN;
    int pedestrian_request = 0;
    
    while(1) {
        // If a keyboard button is pressed, it means that pedestrian button was pressed.
        // Let pedestrians walk along the street
        if(button_pressed()) {
            getchar(); // Cleans cache, it's necessary to allow to program to stop reading an keyboard button
            pedestrian_request = 1;
        }

        switch(state) {
            case(RED):
                printf("RED\n");
                // Securizes that it's printed inmediately
                fflush(stdout); 

                // Waits 5 seconds in intervals of 1 second
                for (int i = 0; i < 5; i++) { 
                    sleep(1);
                }
                state = GREEN;

                // Make flag 0 for letting pedestrian button work well
                pedestrian_request = 0;
                break;
            case(GREEN):
                printf("GREEN\n");
                // Securizes that it's printed inmediately
                fflush(stdout);

                // Waits 5 seconds in intervals of 1 second
                for (int i = 0; i < 5; i++) {
                    if (button_pressed()) {
                        // Cleans input buffer, it allows to stop waiting for any input of keyboard
                        getchar(); 
                        pedestrian_request = 1;

                        // Allows to print the yellow colour
                        break; 
                    }
                    sleep(1);
                }

                // Let pedestrians walk, make semaphore stop
                if(pedestrian_request) {
                    state = YELLOW;
                } else {
                    // If not, prints yellow in 5 seconds
                    state = YELLOW;
                }
                break;
            case(YELLOW):
                printf("YELLOW\n");
                // Securizes that it's printed inmediately
                fflush(stdout);
                sleep(2);
                state = RED;
                break;
        }

    }
}
