/*
 * The function assumes the following:
 * - the userspace governor is always set (done by fcf_init_once)
 * - no two threads will call fcf_set_frequency on the same cpu concurrently
 *   nothing really bad will happen if you do, but you might not set the right frequency
 * - cpu is an actual cpu number, never -1 or something stupid
 * - target_frequency is an actual available frequency number
 * - init is called at least once before
 * - finalize will be called once after
 * 
 * returns the set frequency on success, negative number on error:
 *   -1: not initialized
 *   -2: invalid cpu selected
 */
long fcf_set_frequency(unsigned int cpu, unsigned long target_frequency);

/*
 * May be called more than one time, but only has an effect once.
 * NOT thread safe!
 * 
 * returns 0 on success, -1 on error
 */
int fcf_init_once();

/*
 * returns 0 on success, -1 on error
 * 
 * Note: There is no cleanup of the set frequency.
 * It is your own responsibility to restore the settings if you want to.
 */
int fcf_finalize();
