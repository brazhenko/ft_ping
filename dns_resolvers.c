#include <netinet/in.h>
#include <string.h>
#include <netdb.h>

/*
 * Function: get_ipaddr_by_name
 * ----------------------------
 *      Makes a DNS resolution of domain name.
 *
 *  name - domain name (e.g. google.com)
 *
 *  out - pointer to an in_addr_t to `return`, returns the first entry
 *      of  getaddrinfo() list.
 *
 *  returns:    0 - success
 *              OTHER - error, use gai_strerror()
 *              to discover a particular error
 */

int get_ipaddr_by_name(const char *name, in_addr_t *out) {
    struct addrinfo hints, *result;
    int errcode;

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_flags |= AI_CANONNAME;

    errcode = getaddrinfo(name, NULL, &hints, &result);
    if (errcode != 0) {
        return errcode;
    }

    *out = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(result);

    return 0;
}


int get_name_by_ipaddr(in_addr_t ip, char *buf, size_t bufsize) {
    static int storage[10] = { 0 };
    static int count = 0;

    return 0;
}
