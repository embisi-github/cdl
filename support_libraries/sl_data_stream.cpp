/*a Copyright Gavin J Stark
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sl_general.h"
#include "sl_token.h"

/*a Defines
 */
#define MAX_ARGS (16)

/*a Types
 */
/*t t_sl_data_stream_type
*/
typedef enum
{
    sl_data_stream_type_non_inverting = 0,
    sl_data_stream_type_inverting = 128, // If inverting, then this occurs after each of the following types - really used to check receive data on a bounce test
    sl_data_stream_type_constant = 0,
    sl_data_stream_type_incrementing = 1,
    sl_data_stream_type_random = 2,
    sl_data_stream_type_inverting_constant = 128,
    sl_data_stream_type_inverting_incrementing = 129,
    sl_data_stream_type_inverting_random = 130,
} t_sl_data_stream_type;

/*t t_sl_data_stream
*/
typedef struct t_sl_data_stream
{
    t_sl_data_stream_type type;
    int desired_length;
    unsigned int header; // Header used for packets from this stream
    unsigned int value; // Used in generating the contents of the data stream
    unsigned int seed;  // Used in generating the contents of the data stream
    unsigned int packet_header; // Packet header of this packet - probably static except bit 0 (?)
    unsigned int packet_length; // Packet length of this packet of data (generated from seed and source length when 'sl_data_stream_start_packet' is called)
    unsigned int packet_data; // Next data word for this packet of data (evaluated on 'sl_data_stream_next_data')
    int num_args;
    unsigned int args[MAX_ARGS]; // extra arguments
} t_sl_data_stream;

/*a Data stream generators
 */
/*f sl_data_stream_start_packet
 */
extern void sl_data_stream_start_packet( t_sl_data_stream *ds )
{
    ds->packet_length = ds->desired_length;
    ds->packet_header = ds->header;
    ds->packet_data = 0;
}

/*f sl_data_stream_next_data_word
 */
extern void sl_data_stream_next_data_word( t_sl_data_stream *ds )
{
    switch (ds->type && 127)
    {
    case sl_data_stream_type_constant:
        ds->value = ds->seed;
        break;
    case sl_data_stream_type_incrementing:
        ds->value = ds->value+1;
        break;
    case sl_data_stream_type_random:
        ds->value = (ds->value << 1) ^ (ds->value>>31) ^ (ds->value>>24) ^ (ds->seed);
        break;
    }
    if (ds->type & sl_data_stream_type_inverting)
    {
        ds->packet_data = ~ds->value;
    }
    else
    {
        ds->packet_data = ds->value;
    }
}

/*f sl_data_stream_next_data_byte
 */
extern void sl_data_stream_next_data_byte( t_sl_data_stream *ds )
{
    sl_data_stream_next_data_word( ds );
    ds->packet_data = ds->packet_data&0xff;
}

/*f sl_data_stream_packet_length
 */
extern int sl_data_stream_packet_length( t_sl_data_stream *ds )
{
    return ds->packet_length;
}

/*f sl_data_stream_packet_header
 */
extern unsigned int sl_data_stream_packet_header( t_sl_data_stream *ds )
{
    return ds->packet_header;
}

/*f sl_data_stream_packet_data_word
 */
extern unsigned int sl_data_stream_packet_data_word( t_sl_data_stream *ds )
{
    return ds->packet_data;
}

/*f sl_data_stream_packet_data_byte
 */
extern unsigned int sl_data_stream_packet_data_byte( t_sl_data_stream *ds )
{
    return ds->packet_data & 0xff;
}

/*f sl_data_stream_args
 */
extern int sl_data_stream_args( t_sl_data_stream *ds, unsigned int **args )
{
    *args = ds->args;
    return ds->num_args;
}

/*f sl_data_stream_create
 */
extern t_sl_data_stream *sl_data_stream_create( char *option_string )
{
    t_sl_data_stream *ds;
    char *string_copy, *arg, *end;
    int k, l;

    ds = (t_sl_data_stream *)malloc(sizeof(t_sl_data_stream));
    ds->type = sl_data_stream_type_incrementing;
    ds->desired_length = 0;
    ds->header = 0;
    ds->seed = 0;
    ds->value = 0;

    if (option_string)
    {
        string_copy = sl_str_alloc_copy( option_string );
        if (string_copy[0]=='(') // We expect '(interval, channel, stream type (i/~i,r/~r...), stream length, stream hdr, stream seed, stream value
        {
            end = string_copy + strlen(string_copy); // Get end of option string
            arg = sl_token_next(0, string_copy+1, end ); // get first token; this puts the ptr in arg, and puts a nul at the end of that first token
            if (arg)
            {
                l = 0;
                if (arg[0]=='~') { l = 1; arg++; }
                if (arg[0]=='c') ds->type = l?sl_data_stream_type_inverting_constant : sl_data_stream_type_constant;
                if (arg[0]=='i') ds->type = l?sl_data_stream_type_inverting_incrementing : sl_data_stream_type_incrementing;
                if (arg[0]=='r') ds->type = l?sl_data_stream_type_inverting_random : sl_data_stream_type_random;
            }
            arg = sl_token_next(1, arg, end ); // get continuation token;
            if (arg) sl_integer_from_token( arg, (int *)&ds->desired_length );
            arg = sl_token_next(1, arg, end ); // get continuation token;
            if (arg) sl_integer_from_token( arg, (int *)&ds->header );
            arg = sl_token_next(1, arg, end ); // get continuation token;
            if (arg) sl_integer_from_token( arg, (int *)&ds->seed );
            arg = sl_token_next(1, arg, end ); // get continuation token;
            if (arg) sl_integer_from_token( arg, (int *)&ds->value );
            arg = sl_token_next(1, arg, end );
            for (k=0; (k<MAX_ARGS) && arg; k++ )
            {
                sl_integer_from_token( arg, (int *)&ds->args[k] );
                arg = sl_token_next(1, arg, end );
            }
            ds->num_args = k;
//             fprintf(stderr, "Added data stream%d %d %08x %d %d\n",
//                     ds->type,
//                     ds->desired_length,
//                     ds->header,
//                     ds->seed,
//                     ds->value );
        }
    }
    return ds;
}
