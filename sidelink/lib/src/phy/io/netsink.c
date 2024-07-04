/**
* Copyright 2013-2019 
* Fraunhofer Institute for Telecommunications, Heinrich-Hertz-Institut (HHI)
*
* This file is part of the HHI Sidelink.
*
* HHI Sidelink is under the terms of the GNU Affero General Public License
* as published by the Free Software Foundation version 3.
*
* HHI Sidelink is distributed WITHOUT ANY WARRANTY,
* without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* A copy of the GNU Affero General Public License can be found in
* the LICENSE file in the top-level directory of this distribution
* and at http://www.gnu.org/licenses/.
*
* The HHI Sidelink is based on srsLTE.
* All necessary files and sources from srsLTE are part of HHI Sidelink.
* srsLTE is under Copyright 2013-2017 by Software Radio Systems Limited.
* srsLTE can be found under:
* https://github.com/srsLTE/srsLTE
*/

/*
 * Copyright 2013-2019 Software Radio Systems Limited
 *
 * This file is part of srsLTE.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>


#include "srslte/phy/io/netsink.h"

int srslte_netsink_init(srslte_netsink_t *q, const char *address, uint16_t port, srslte_netsink_type_t type) {
  bzero(q, sizeof(srslte_netsink_t));

  q->sockfd=socket(AF_INET, type==SRSLTE_NETSINK_TCP?SOCK_STREAM:SOCK_DGRAM,0);
  if (q->sockfd < 0) {
    perror("socket");
    return -1;
  }

  int enable = 1;
#if defined (SO_REUSEADDR)
  if (setsockopt(q->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
      perror("setsockopt(SO_REUSEADDR) failed");
#endif
#if defined (SO_REUSEPORT)
  if (setsockopt(q->sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
      perror("setsockopt(SO_REUSEPORT) failed");
#endif

  q->servaddr.sin_family = AF_INET;
  q->servaddr.sin_addr.s_addr=inet_addr(address);
  q->servaddr.sin_port=htons(port);
  q->connected = false;
  q->type = type;

  return 0;
}

void srslte_netsink_free(srslte_netsink_t *q) {
  if (q->sockfd) {
    close(q->sockfd);
  }
  bzero(q, sizeof(srslte_netsink_t));
}

int srslte_netsink_set_nonblocking(srslte_netsink_t *q) {
  if (fcntl(q->sockfd, F_SETFL, O_NONBLOCK)) {
    perror("fcntl");
    return -1;
  }
  return 0;
}

int srslte_netsink_write(srslte_netsink_t *q, void *buffer, int nof_bytes) {
  if (!q->connected) {
    printf("srslte_netsink_write not connected, dropping packet\n");
    // if (connect(q->sockfd,&q->servaddr,sizeof(q->servaddr)) < 0) {
    //   if (errno == ECONNREFUSED || errno == EINPROGRESS) {
    //     return 0;
    //   } else {
    //     perror("srslte_netsink_write::connect");
    //     exit(-1);
    //     return -1;
    //   }
    // } else {
    //   q->connected = true;
    // }
  }
  int n = 0;
  if (q->connected) {
    n = write(q->sockfd, buffer, nof_bytes);
    if (n < 0) {
      if (errno == ECONNRESET) {
        close(q->sockfd);
        q->sockfd=socket(AF_INET, q->type==SRSLTE_NETSINK_TCP?SOCK_STREAM:SOCK_DGRAM,0);
        if (q->sockfd < 0) {
          perror("socket");
          return -1;
        }
        q->connected = false;
        return 0;
      }
    }
  }
  return n;
}


/**
 * @brief Reads data from a sink, which actually acts a source
 *
 * This has been added to retrieve data as a TCP client
 *
 * @param q
 * @param buffer
 * @param nof_bytes
 * @return int
 */
int srslte_netsink_read(srslte_netsink_t *q, void *buffer, int nof_bytes) {
  bool init_connect = !q->connected;
  if (!q->connected) {
    if (connect(q->sockfd,&q->servaddr,sizeof(q->servaddr)) < 0) {
      int _errno = errno;
      if (_errno == ECONNREFUSED || _errno == EINPROGRESS || _errno == EALREADY) {
        return 0;
      } else {
        printf("Errno: %d\n", _errno);
        perror("srslte_netsink_read::connect");
        return 0;//-1;
      }
    } else {
      // printf("Connected to server\n");
      q->connected = true;
    }
  }
  int n = 0;
  if (q->connected && nof_bytes > 0) {

    n = read(q->sockfd, buffer, nof_bytes);
    if (n == 0) {

      // only report closing when we didnot open during same call
      if(!init_connect) {
        printf("Connection closed\n");
      }

      close(q->sockfd);

      // re-create socket
      q->sockfd=socket(AF_INET, q->type==SRSLTE_NETSINK_TCP?SOCK_STREAM:SOCK_DGRAM,0);
      if (q->sockfd < 0) {
        perror("socket");
        return -1;
      }

      // make it non-blocking
      if (fcntl(q->sockfd, F_SETFL, O_NONBLOCK)) {
        perror("fcntl");
        return -1;
      }

      q->connected = false;
      return 0;
    }
    // ignore error, when we are running in non-blocking mode
    if (EAGAIN != errno && n == -1) {
      perror("read");
    }
  }

  // report connection state incase is did not close immediately
  if (q->connected && init_connect) {
    printf("Connected to server\n");
  }
  return n;
}
