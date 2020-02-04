
#include <sysdep.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <urjtag/cable.h>
#include <urjtag/chain.h>

///////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////
static int xvc_connect(urj_cable_t *cable, const urj_param_t *params[])
{
	//Connect to TCP server
	return URJ_STATUS_OK;
}

/** @return URJ_STATUS_OK on success; URJ_STATUS_FAIL on failure */
static int xvc_init(urj_cable_t *cable)
{
	return URJ_STATUS_OK;
}


///////////////////////////////////////////////////////////////////
// Cleanup
///////////////////////////////////////////////////////////////////
static void xvc_disconnect(urj_cable_t *cable)
{
	// Clean up if a disconnection was detected by the low level driver.
	// Needs to call chain_disconnect, which will call done() then cable_free()
}

static void xvc_free(urj_cable_t *cable)
{
	// Cleanup eventually extra allocated memory, etc.
}

static void xvc_done(urj_cable_t *cable)
{
	// Drive the hardware to a safe and consistent state

}

///////////////////////////////////////////////////////////////////
// set bitrate for shifting data through the chain
///////////////////////////////////////////////////////////////////
static void xvc_set_frequency(urj_cable_t *cable, uint32_t freq)
{
	// send tclk command
}

///////////////////////////////////////////////////////////////////
// immediate JTAG activities
///////////////////////////////////////////////////////////////////
static void xvc_clock(urj_cable_t *cable, int tms, int tdi, int n)
{
	// low-level
}

/** @return 0 or 1 on success; -1 on failure */
static int xvc_get_tdo(urj_cable_t *cable)
{
	// low-level
	return 0;
}

/** @return nonnegative number, or the number of transferred bits on
 * success; -1 on failure
 */
static int xvc_transfer(urj_cable_t *cable, int len, const char *in, char *out)
{
	// TMS is set to zero durring transfers
	return URJ_STATUS_OK;
}


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/** @return 0 or 1 on success; -1 on failure */
static int xvc_set_signal(urj_cable_t *cable, int mask, int val)
{
	return URJ_STATUS_OK;
}

/** @return 0 or 1 on success; -1 on failure */
static int xvc_get_signal(urj_cable_t *cable, urj_pod_sigsel_t sig)
{
	return URJ_STATUS_OK;
}


///////////////////////////////////////////////////////////////////
// internally used to actually perform JTAG activities
///////////////////////////////////////////////////////////////////
static void xvc_flush(urj_cable_t *cable, urj_cable_flush_amount_t how_much)
{
	// Use to execute queued activity
	// See generic.c:urj_tap_cable_generic_flush_using_transfer
}

///////////////////////////////////////////////////////////////////
// a help text to be displayed by the jtag command shell
///////////////////////////////////////////////////////////////////
static void xvc_help(urj_log_level_t ll, const char *cablename)
{

}



const urj_cable_driver_t urj_tap_cable_xvc_driver = {
    "xvc",
    N_("Xilinx Virtual Cable JTAG Connection"),
    URJ_CABLE_DEVICE_OTHER,
    { .other = xvc_connect, },
    xvc_disconnect,
    xvc_free,
    xvc_init,
    xvc_done,
    xvc_set_frequency,
    xvc_clock,
    xvc_get_tdo,
    xvc_transfer,
    xvc_set_signal,
    xvc_get_signal,
    xvc_flush,
    xvc_help
};
