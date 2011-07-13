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
    int i;
    for (i=0; i<100000; i++);
    //printf("Done print '%s'\n", text );
}
extern int main( int argc, char **argv )
{
    const char *thread_names[] = 
    {
        "main",
        "bee1",
        "bee2",
        "bee3",
        "bee4",
        "bee5",
        "bee6",
        "bee7",
        "bee8",
        NULL
    };

    t_sl_wl_thread_pool_ptr pool;
    t_sl_wl_worklist *worklist;
    int i, j;
    int num_threads;
    int work_item=0;
    int work_per_thread=8;

    pool = sl_wl_create_thread_pool();
    num_threads = 0;
    for (i=0; thread_names[i]; i++)
    {
        if (i>0)
        {
            printf("Spawn %d\n",sl_wl_spawn_worker_thread( pool, thread_names[i], NULL ));
        }
        num_threads++;
    }

    worklist = sl_wl_add_worklist( pool, "prints", num_threads*work_per_thread, 1 );

    for (j=0; j<work_per_thread; j++)
    {
        for (i=0; i<num_threads; i++)
        {
            char buffer[256];
            sprintf( buffer, "work.%d.%d", i, j );
            const char *text = (const char *)sl_str_alloc_copy( buffer );
            sl_wl_set_work_head( worklist, work_item, thread_names[i], text, NULL );
            sl_wl_set_work_item( worklist, work_item, 0, do_print, (void *)text );
            sl_wl_assign_work_to_thread( worklist, work_item, thread_names[i] );
            work_item++;
        }
    }

    for (i=0; i<40000; i++)
    {
        sl_wl_execute_worklist( worklist, NULL, 0 );
    }

    //printf("Terminate %d\n",sl_wl_terminate_worker_thread( pool, "bee1" ));
    return 0;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

