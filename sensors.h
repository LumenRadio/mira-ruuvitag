#ifndef SENSORS_H
#define SENSORS_H

/**
 * Startup sensors process
 *
 * The sensors process is responsible for polling sensor values send the result
 * to the root node.
 *
 * It should also provide configuration and feedback via NFC
 */
void sensors_init(
    void);

#endif
