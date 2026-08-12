//
//  socketpair.h
//  TransDB
//
//  Created by Miroslav Kudrnac on 09.10.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#ifndef TransDB_socketpair_h
#define TransDB_socketpair_h

#ifdef __cplusplus
extern "C" {
#endif

/** Description in .c file
 */
    int dumb_socketpair(SOCKET socks[2], int);
    
#ifdef __cplusplus
}
#endif
    
#endif
