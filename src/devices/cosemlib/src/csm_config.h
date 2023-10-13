#ifndef CSM_CONFIG_H
#define CSM_CONFIG_H

//#include <assert.h>
//#include <stdio.h>

#define CSM_DEF_PDU_SIZE        1024

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef CSM_ASSERT
#define CSM_ASSERT(condition) assert(condition)
#endif

#ifndef CSM_TRACE
#define CSM_TRACE(...) printf(__VA_ARGS__)
#endif

#ifndef CSM_LOG
#define CSM_LOG(...) printf("[LOG]");printf(__VA_ARGS__);printf("\r\n")
#endif

#ifndef CSM_ERR
#define CSM_ERR(...) printf("[ERR]");printf(__VA_ARGS__);printf("\r\n")
#endif

#endif // CSM_CONFIG_H

