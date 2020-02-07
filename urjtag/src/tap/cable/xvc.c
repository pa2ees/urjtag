
#include <sysdep.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <urjtag/cable.h>
#include <urjtag/chain.h>
#include "generic.h"


static const short XVC_PORT = 2542;
#define BUFF_SIZE 2048


typedef struct
{
    int sockfd;
    int max_bytes;
    int valid_bits;
    char* tms;
    char* tdi;
    char* tdo;
} xvc_parameters_t;

///////////////////////////////////////////////////////////////////
// a help text to be displayed by the jtag command shell
///////////////////////////////////////////////////////////////////
static void xvc_help(urj_log_level_t ll, const char *cablename)
{
	urj_log(ll, "Usage: cable %s IPADDRESS", cablename);
}

///////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////
static int xvc_connect(urj_cable_t *cable, const urj_param_t *params[])
{
    xvc_parameters_t *cable_params;
    struct sockaddr_in sa;
	const char* address;

    if (params == NULL || params[0]->key != URJ_CABLE_PARAM_KEY_ADDRESS)
    {
        urj_error_set (URJ_ERROR_SYNTAX, _("missing required address\n"));
        xvc_help (URJ_ERROR_SYNTAX, "xvc");
        return URJ_STATUS_FAIL;
    }
    address = params[0]->value.string;

    cable_params = calloc (1, sizeof (*cable_params));
    if (!cable_params)
    {
        urj_error_set (URJ_ERROR_OUT_OF_MEMORY, _("calloc(%zd) fails"),
                       sizeof (*cable_params));
        free (cable);
        return URJ_STATUS_FAIL;
    }

    cable_params->max_bytes = 0;
    cable_params->valid_bits = 0;

    urj_log (URJ_LOG_LEVEL_NORMAL,_("Connecting to XVC server\n"));


    if ((cable_params->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        urj_error_set(URJ_ERROR_NOTFOUND, _("Unable to setup TCP socket"));
        return URJ_STATUS_FAIL;
    }

    sa.sin_family = AF_INET;      /* host byte order */
    sa.sin_port = htons(XVC_PORT);    /* short, network byte order */
    inet_pton(AF_INET, address, &(sa.sin_addr));
    bzero(&(sa.sin_zero), 8);     /* zero the rest of the struct */

	urj_log (URJ_LOG_LEVEL_COMM, _("Connecting to server: %s\n"), address);

    if (-1 == connect(cable_params->sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr))) {
        perror("connect");
        urj_error_set(URJ_ERROR_NOTFOUND, _("Unable to connect"));
        return URJ_STATUS_FAIL;
    }

    cable->delay = 1000;
    cable->chain = NULL;
    cable->params = cable_params;

    return URJ_STATUS_OK;
}

static void
print_bit_vector (urj_log_level_t ll, int len, char *vec)
{
    int ii;
    int numbytes = (len + 7) / 8;

    if (! ((ll) >= urj_log_state.level))
        return;

    for (ii = 0; ii < numbytes; ii++)
    {
        printf("%02x ", 0xFF & vec[ii]);
    }
    printf("|\t");
    for (ii = 0; ii < len; ii++)
    {
        int bit = ii&0x7;
        int byte = ii/8;
        int val = (vec[byte] >> bit) & 0x1;
        printf("%d ", val);
    }
    printf("\n");
}

static int xvc_send(urj_cable_t *cable, int bits)
{
    xvc_parameters_t* cable_params = (xvc_parameters_t*) cable->params;
    int numbytes;
    int ret = 0;

    numbytes = (bits + 7) / 8;
    ret |= send(cable_params->sockfd, "shift:", 6, MSG_MORE);
    ret |= send(cable_params->sockfd, &bits, 4, MSG_MORE);
    ret |= send(cable_params->sockfd, cable_params->tms, numbytes, MSG_MORE);
    ret |= send(cable_params->sockfd, cable_params->tdi, numbytes, 0);

    urj_log (URJ_LOG_LEVEL_COMM, _("TX(%d):\t"), bits);
    print_bit_vector(URJ_LOG_LEVEL_COMM, bits, cable_params->tdi);
    if (ret)
    {
        urj_error_set(URJ_ERROR_FILEIO, _("Send to XVC failed"));
    }
    numbytes = recv(cable_params->sockfd, cable_params->tdo, cable_params->max_bytes, 0);
    if (numbytes < 0)
    {
        urj_error_set(URJ_ERROR_FILEIO, _("RX from XVC failed"));
    }

    urj_log (URJ_LOG_LEVEL_COMM, "RX(%d):\t", bits);
    print_bit_vector(URJ_LOG_LEVEL_COMM, bits, cable_params->tdo);
    return URJ_STATUS_OK;
}

/** @return URJ_STATUS_OK on success; URJ_STATUS_FAIL on failure */
static int xvc_init(urj_cable_t *cable)
{
    xvc_parameters_t* cable_params = (xvc_parameters_t*) cable->params;
    int fd = cable_params->sockfd;
    int numbytes;
    char buf[256];

    if (send(fd, "getinfo:", 8, 0))
    {
        urj_error_set(URJ_ERROR_FILEIO, _("Send to XVC failed"));
    }
    numbytes = recv(fd, buf, 256, 0);
    if (numbytes < 0)
    {
        urj_error_set(URJ_ERROR_FILEIO, _("RX from XVC failed"));
    }
    urj_log(URJ_LOG_LEVEL_COMM, "TX: getinfo\tRX:%s", buf);

    if (strstr(buf, "xvcServer_v1.0"))
    {
        char *buffersize = strstr(buf, ":") + 1;
        int buff_size = strtol(buffersize, NULL, 0);
        cable_params->max_bytes = buff_size;
        cable_params->tms = calloc (1, buff_size);
        cable_params->tdi = calloc (1, buff_size);
        cable_params->tdo = calloc (1, buff_size);
        urj_log(URJ_LOG_LEVEL_COMM, "Max buffer size = %d\n", buff_size);
        return URJ_STATUS_OK;
    }
    return URJ_STATUS_FAIL;
}


///////////////////////////////////////////////////////////////////
// Cleanup
///////////////////////////////////////////////////////////////////
static void xvc_done(urj_cable_t *cable)
{
    // Drive the hardware to a safe and consistent state
    xvc_parameters_t* cable_params = (xvc_parameters_t*) cable->params;
    // Reset the TAP?
    urj_log(URJ_LOG_LEVEL_COMM, _("Closing XVC connection\n"));
    close(cable_params->sockfd);
}

static void xvc_disconnect(urj_cable_t *cable)
{
    urj_log(URJ_LOG_LEVEL_COMM, _("Detected disconnect\n"));

    // Clean up if a disconnection was detected by the low level driver.
    // Needs to call chain_disconnect, which will call done() then cable_free()
    urj_tap_chain_disconnect(cable->chain);
    xvc_done(cable);
}

static void xvc_free(urj_cable_t *cable)
{
    // Cleanup extra allocated memory, etc.
    xvc_parameters_t* cable_params = (xvc_parameters_t*) cable->params;
    urj_log(URJ_LOG_LEVEL_COMM, _("Freeing XVC memory\n"));
    free(cable_params->tms);
    free(cable_params->tdi);
    free(cable_params->tdo);
    free(cable_params);
    free(cable);
}

///////////////////////////////////////////////////////////////////
// set bitrate for shifting data through the chain
///////////////////////////////////////////////////////////////////
static void xvc_set_frequency(urj_cable_t *cable, uint32_t freq)
{
    urj_log(URJ_LOG_LEVEL_COMM, _("Set XVC clock rate\n"));
    // send tclk command
}

///////////////////////////////////////////////////////////////////
// immediate JTAG activities
///////////////////////////////////////////////////////////////////
static void xvc_clock(urj_cable_t *cable, int tms, int tdi, int n)
{
    xvc_parameters_t* cable_params = (xvc_parameters_t*) cable->params;
    long tms_vec = 0;
    long tdi_vec = 0;
    int bits;


    urj_log(URJ_LOG_LEVEL_COMM, _("Send tms=%x, tdi=%x, n=%d\n"), tms, tdi, n);
    bits = n;

    if (bits > 32)
    {
        if (tms)
            tms_vec = -1;
        if (tdi)
            tdi_vec = -1;
        memcpy(cable_params->tms, &tms_vec, sizeof(tms_vec));
        memcpy(cable_params->tdi, &tdi_vec, sizeof(tms_vec));
        while (bits >= 32)
        {
            xvc_send(cable, 32);
            bits -= 32;
        }
    }

    // leftovers
    if (tms)
        tms_vec = (1<<bits) -1;
    if (tdi)
        tdi_vec = (1<<bits) -1;
    memcpy(cable_params->tms, &tms_vec, sizeof(tms_vec));
    memcpy(cable_params->tdi, &tdi_vec, sizeof(tms_vec));
    xvc_send(cable, bits);
}

/** @return 0 or 1 on success; -1 on failure */
static int xvc_get_tdo(urj_cable_t *cable)
{
    xvc_parameters_t* cable_params = (xvc_parameters_t*) cable->params;
    char tdo = cable_params->tdo[0];
    int tdo_bit = tdo & 0x1;
    urj_log(URJ_LOG_LEVEL_COMM, _("get tdo: %x (%d)\n"), tdo, tdo_bit);
    return tdo_bit;
}

static void add_bit(char* bit_vector, int bit_number, char value)
{
    int bit = bit_number&0x7;
    int byte = bit_number/8;
    if (value)
        bit_vector[byte] |= 1 << bit;
    else
        bit_vector[byte] &= ~(1 << bit);
}

static char get_bit(char* bit_vector, int bit_number)
{
    int bit = bit_number&0x7;
    int byte = bit_number/8;
    char ret =  (bit_vector[byte] >> bit) & 0x1;
    return ret;
}

/** @return nonnegative number, or the number of transferred bits on
 * success; -1 on failure
 */
static int xvc_transfer(urj_cable_t *cable, int len, const char *in, char *out)
{
    int numbytes = (len + 7) / 8;
    int ii;
    xvc_parameters_t* cable_params = (xvc_parameters_t*) cable->params;

    if (!len)
        return URJ_STATUS_FAIL;

    // TMS is set to zero during 'transfer'
    memset(cable_params->tms, 0, numbytes);

    for(ii = 0; ii < len; ii++)
    {
        add_bit(cable_params->tdi, ii, in[ii]);
    }
    urj_log(URJ_LOG_LEVEL_COMM, _("transfer: %d (%d)\n"), len, numbytes);

    xvc_send(cable, len);

    if (out)
    {
        for(ii = 0; ii < len; ii++)
        {
            out[ii] = get_bit(cable_params->tdo, ii);
        }
    }
    return URJ_STATUS_OK;
}


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

/** @return 0 or 1 on success; -1 on failure */
static int xvc_set_signal(urj_cable_t *cable, int mask, int val)
{
    return URJ_STATUS_FAIL;
}

/** @return 0 or 1 on success; -1 on failure */
static int xvc_get_signal(urj_cable_t *cable, urj_pod_sigsel_t sig)
{
    return URJ_STATUS_FAIL;
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
    urj_tap_cable_generic_flush_using_transfer,
    xvc_help
};
