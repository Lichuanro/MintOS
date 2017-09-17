
/* Persistent state for the robust I/O (Mint) package */
/* $begin mint_t */
#define MINT_BUFSIZE 8192
typedef struct {
    int mint_fd;                /* Descriptor for this internal buf */
    int mint_cnt;               /* Unread bytes in internal buf */
    char *mint_bufptr;          /* Next unread byte in internal buf */
    char mint_buf[MINT_BUFSIZE]; /* Internal buffer */
} mint_t;
/* $end mint_t */


/****************************************
 * The Mint package - Robust I/O functions
 ****************************************/

/*
 * mint_readn - Robustly read n bytes (unbuffered)
 */
/* $begin mint_readn */
ssize_t mint_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* Interrupted by sig handler return */
		nread = 0;      /* and call read() again */
	    else
		return -1;      /* errno set by read() */
	}
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* Return >= 0 */
}
/* $end mint_readn */

/*
 * mint_writen - Robustly write n bytes (unbuffered)
 */
/* $begin mint_writen */
ssize_t mint_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* Interrupted by sig handler return */
		nwritten = 0;    /* and call write() again */
	    else
		return -1;       /* errno set by write() */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}
/* $end mint_writen */


/*
 * mint_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, mint_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    mint_cnt is the number of unread bytes in the internal buffer. On
 *    entry, mint_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin mint_read */
static ssize_t mint_read(mint_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->mint_cnt <= 0) {  /* Refill if buf is empty */
	rp->mint_cnt = read(rp->mint_fd, rp->mint_buf,
			   sizeof(rp->mint_buf));
	if (rp->mint_cnt < 0) {
	    if (errno != EINTR) /* Interrupted by sig handler return */
		return -1;
	}
	else if (rp->mint_cnt == 0)  /* EOF */
	    return 0;
	else
	    rp->mint_bufptr = rp->mint_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->mint_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->mint_cnt < n)
	cnt = rp->mint_cnt;
    memcpy(usrbuf, rp->mint_bufptr, cnt);
    rp->mint_bufptr += cnt;
    rp->mint_cnt -= cnt;
    return cnt;
}
/* $end mint_read */

/*
 * mint_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin mint_readinitb */
void mint_readinitb(mint_t *rp, int fd)
{
    rp->mint_fd = fd;
    rp->mint_cnt = 0;
    rp->mint_bufptr = rp->mint_buf;
}
/* $end mint_readinitb */

/*
 * mint_readnb - Robustly read n bytes (buffered)
 */
/* $begin mint_readnb */
ssize_t mint_readnb(mint_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = mint_read(rp, bufp, nleft)) < 0)
            return -1;          /* errno set by read() */
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end mint_readnb */

/*
 * mint_readlineb - Robustly read a text line (buffered)
 */
/* $begin mint_readlineb */
ssize_t mint_readlineb(mint_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = mint_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;
	    if (c == '\n') {
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, no data read */
	    else
		break;    /* EOF, some data was read */
	} else
	    return -1;	  /* Error */
    }
    *bufp = 0;
    return n-1;
}
/* $end mint_readlineb */

/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t Mint_readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = mint_readn(fd, ptr, nbytes)) < 0)
	    printf("Mint_readn error");
    return n;
}

void Mint_writen(int fd, void *usrbuf, size_t n)
{
    if (mint_writen(fd, usrbuf, n) != n)
	printf("Mint_writen error");
}

void Mint_readinitb(mint_t *rp, int fd)
{
    mint_readinitb(rp, fd);
}

ssize_t Mint_readnb(mint_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = mint_readnb(rp, usrbuf, n)) < 0)
	printf("Mint_readnb error");
    return rc;
}

ssize_t Mint_readlineb(mint_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = mint_readlineb(rp, usrbuf, maxlen)) < 0)
	printf("Mint_readlineb error");
    return rc;
}

void getchar(char* result)
{
    read(0,result,2);
}

void gets(char * result)
{
    read(0,result,80);
    int i;

    for(i = 0 ; i < 80 ; i++)
    {
        if(result[i] == '\n')
            result[i] = 0;
    }
}

void puts(char* result)
{
    printf("%s",result);
}

int fgets(int fd, char * result, int length) {
  int n = read(fd, result, length);
  return n;
}
