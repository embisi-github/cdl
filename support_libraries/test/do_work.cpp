/*a Copyright
*/
/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "sl_work_list.h"

/*a Defines
 */
#if 0
#define WHERE_I_AM {fprintf(stderr,"%s:%d\n",__func__,__LINE__ );}
#define WHERE_I_AM_TH {fprintf(stderr,"%p:%s:%d\n",this_thread,__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#define WHERE_I_AM_TH {}
#endif

/*a Types
 */
/*a Main
 */
static void do_print( void *handle )
{
    char *text = (char *)handle;
    printf("Done print '%s'\n", text );
}
extern int main( int argc, char **argv )
{
    t_sl_wl_thread_pool_ptr pool;
    t_sl_wl_worklist *worklist;
    int i;
    pool = sl_wl_create_thread_pool();
    printf("Spawn %d\n",sl_wl_spawn_worker_thread( pool, "bee1", NULL ));

    worklist = sl_wl_add_worklist( pool, "prints", 4, 1 );
    sl_wl_set_work_head( worklist, 0, "main", "one", NULL );
    sl_wl_set_work_item( worklist, 0, 0, do_print, (void *)"work_one" );
    sl_wl_set_work_head( worklist, 1, "main", "two", NULL );
    sl_wl_set_work_item( worklist, 1, 0, do_print, (void *)"work_two" );
    sl_wl_set_work_head( worklist, 2, "sub", "one", NULL );
    sl_wl_set_work_item( worklist, 2, 0, do_print, (void *)"work_three" );
    sl_wl_set_work_head( worklist, 3, "sub", "two", NULL );
    sl_wl_set_work_item( worklist, 3, 0, do_print, (void *)"work_four" );

    sl_wl_assign_work_to_thread( worklist, 1, "bee1" );
    for (i=0; i<400*100000; i++)
    {
        sl_wl_execute_worklist( worklist, NULL, 0 );
    }

    printf("Terminate %d\n",sl_wl_terminate_worker_thread( pool, "bee1" ));
    return 0;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

