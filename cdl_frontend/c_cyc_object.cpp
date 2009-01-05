/*a Copyright
  
  This file 'c_cyc_object.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a Include
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "lexical_types.h"
#include "c_lexical_analyzer.h"
#include "c_cyc_object.h"

/*a Constructors and destructors
 */
/*f c_cyc_object::c_cyc_object
 */
c_cyc_object::c_cyc_object()
{
    next_in_list = NULL;
    last_in_list = this;
    co_type = co_type_none;
    co_textual_type = "<None yet>";
    delete_fn = NULL;
    co_next_link = 0;
    seed = random()&0xfff;
    parent = NULL;
    lex_reset_file_posn( &start_posn );
    lex_reset_file_posn( &end_posn );
}

/*f c_cyc_object::c_cyc_object
 */
c_cyc_object::~c_cyc_object()
{
    int i;
    for (i=0; i<co_next_link; i++)
    {
        if (co_links[i].compile_stage == co_compile_stage_parse)
        {
            switch (co_links[i].link_type)
            {
            case co_link_type_object:
                if (co_links[i].data.object)
                {
                    delete(co_links[i].data.object);
                }
                break;
            case co_link_type_string:
            case co_link_type_symbol:
            case co_link_type_sized_int:
                break;
            }
        }
    }
    if (next_in_list)
    {
        delete(next_in_list);
    }
    if (delete_fn)
    {
        delete_fn( this );
    }
}

/*f c_cyc_object::co_init
 */
void c_cyc_object::co_init( t_co_type co_type, const char *string )
{
    this->co_type = co_type;
    this->co_textual_type = string;
    SL_DEBUG( sl_debug_level_verbose_info, "New object type %d:%s: %p", co_type, string, this );
}

/*a Handle functions
 */
/*f object_from_handle
 */
c_cyc_object *object_from_handle( char *buffer )
{
    int i, value, chk;
    c_cyc_object *co;

    if (strncmp(buffer, "obj", 3))
        return NULL;

    for (i=0; i<13; i++)
        if (!buffer[i])
            return NULL;

    chk = 0;
    value = 0;
    for (i=0; i<6; i++)
    {
        if ((buffer[3+i]<48) || (buffer[3+i]>111))
            return NULL;
        value |= (buffer[3+i]-48)<<(6*i);
        chk += buffer[3+i];
    }
    if ((chk&0x3f)!=(buffer[9]-48))
        return NULL;

    co = (c_cyc_object *)value;
    value = 0;
    for (i=0; i<3; i++)
    {
        if ( (buffer[10+i]<'0') ||
             ( (buffer[10+i]>'9') && (buffer[10+i]<'a') ) ||
             (buffer[10+i]>'f') )
            return NULL;
        if (buffer[10+i]<='9')
            value = 16*value + (buffer[10+i]-'0');
        else
            value = 16*value + 10 + (buffer[10+i]-'a');
    }
    if (!co->check_seed(value))
        return NULL;
    return co;
}

/*f c_cyc_object::get_handle
 */
void c_cyc_object::get_handle( char *buffer )
{
    unsigned int i, chk;
    long int value;

    strcpy( buffer, "obj" );
    value = (long int)this;
    chk = 0;
    for (i=0; i<(sizeof(long int)*8+5)/6; i++)
    {
        buffer[3+i]=48+(value&0x3f);
        chk += buffer[3+i];
        value>>=6;
    }
    buffer[3+i] = 48+(chk&0x3f);
    sprintf(buffer+4+i,"%03x",seed);
}

/*f c_cyc_object::check_seed
 */
int c_cyc_object::check_seed( int seed )
{
    return seed==this->seed;
}

/*a Linking
 */
/*f c_cyc_object::co_set_file_bound( c_cyc_object, c_cyc_object )
 */
void c_cyc_object::co_set_file_bound( c_cyc_object *start_object, c_cyc_object *end_object )
{
    if (start_object)
        this->start_posn = start_object->start_posn;
    if (end_object)
        this->end_posn = end_object->last_in_list->end_posn;
}

/*f c_cyc_object::co_set_file_bound( c_cyc_object, t_lex_file_posn )
 */
void c_cyc_object::co_set_file_bound( c_cyc_object *start_object, t_lex_file_posn end_lex_file_posn )
{
    if (start_object)
        this->start_posn = start_object->start_posn;
    this->end_posn = end_lex_file_posn;
}

/*f c_cyc_object::co_set_file_bound( t_lex_file_posn, c_cyc_object )
 */
void c_cyc_object::co_set_file_bound( t_lex_file_posn start_lex_file_posn, c_cyc_object *end_object )
{
    this->start_posn = start_lex_file_posn;
    if (end_object)
        this->end_posn = end_object->last_in_list->end_posn;
}

/*f c_cyc_object::co_set_file_bound( t_lex_file_posn, t_lex_file_posn )
 */
void c_cyc_object::co_set_file_bound( t_lex_file_posn start_lex_file_posn, t_lex_file_posn end_lex_file_posn )
{
    this->start_posn = start_lex_file_posn;
    this->end_posn = end_lex_file_posn;
}

/*f c_cyc_object::co_link( c_cyc_object )
 */
void c_cyc_object::co_link( t_co_compile_stage compile_stage, c_cyc_object *object, const char *string )
{
    if (this->co_next_link < CO_MAX_LINK)
    {
        this->co_links [ this->co_next_link ].link_type = co_link_type_object;
        this->co_links [ this->co_next_link ].compile_stage = compile_stage;
        this->co_links [ this->co_next_link ].string = string;
        this->co_links [ this->co_next_link ].data.object = object;
        this->co_next_link++;
        if (compile_stage==co_compile_stage_parse)
        {
            while (object)
            {
                object->parent = this;
                object = object->next_in_list;
            }
        }
    }
    else
    {
        fprintf( stderr, "Too many links - fault in cyclicity source code (%s)\n", co_textual_type );
        exit(4);
    }
}

/*f c_cyc_object::co_link( string )
 */
void c_cyc_object::co_link( t_co_compile_stage compile_stage, t_string *tstring, const char *string )
{
    if (this->co_next_link < CO_MAX_LINK)
    {
        this->co_links [ this->co_next_link ].link_type = co_link_type_string;
        this->co_links [ this->co_next_link ].compile_stage = compile_stage;
        this->co_links [ this->co_next_link ].string = string;
        this->co_links [ this->co_next_link ].data.string = tstring;
        this->co_next_link++;
        if ( (tstring) && (compile_stage==co_compile_stage_parse) )
        {
            tstring->user = this;
        }
    }
    else
    {
        fprintf( stderr, "Too many links - fault in cyclicity source code (%s)\n", co_textual_type );
        exit(4);
    }
}

/*f c_cyc_object::co_link( symbol )
 */
void c_cyc_object::co_link( t_co_compile_stage compile_stage, t_symbol *symbol, const char *string )
{
    if (this->co_next_link < CO_MAX_LINK)
    {
        this->co_links [ this->co_next_link ].link_type = co_link_type_symbol;
        this->co_links [ this->co_next_link ].compile_stage = compile_stage;
        this->co_links [ this->co_next_link ].string = string;
        this->co_links [ this->co_next_link ].data.symbol = symbol;
        this->co_next_link++;
        if ( (symbol) && (compile_stage==co_compile_stage_parse) )
        {
            symbol->user = this;
        }
    }
    else
    {
        fprintf( stderr, "Too many links - fault in cyclicity source code (%s)\n", co_textual_type );
        exit(4);
    }
}

/*f c_cyc_object::co_link( sized_int )
 */
void c_cyc_object::co_link( t_co_compile_stage compile_stage, t_sized_int *sized_int, const char *string )
{
    if (this->co_next_link < CO_MAX_LINK)
    {
        this->co_links [ this->co_next_link ].link_type = co_link_type_sized_int;
        this->co_links [ this->co_next_link ].compile_stage = compile_stage;
        this->co_links [ this->co_next_link ].string = string;
        this->co_links [ this->co_next_link ].data.sized_int = sized_int;
        this->co_next_link++;
        if ( (sized_int) && (compile_stage==co_compile_stage_parse) )
        {
            sized_int->user = this;
        }
    }
    else
    {
        fprintf( stderr, "Too many links - fault in cyclicity source code (%s)\n", co_textual_type );
        exit(4);
    }
}

/*f c_cyc_object::co_link_symbol_list( compile_stage, symbol_list null terminated )
 */
void c_cyc_object::co_link_symbol_list( t_co_compile_stage compile_stage, ... ) 
{
    va_list ap;
    t_symbol *symbol;

    va_start( ap, compile_stage );

    symbol = (t_symbol *)va_arg( ap, t_symbol *);
    while ( symbol )
    {
        co_link( compile_stage, symbol, "<listed>" );
        symbol = (t_symbol *)va_arg( ap, t_symbol *);
    }

    va_end( ap );
}


/*f c_cyc_object::display_links
 */
static void display_indent( int indent )
{
    int i;
    for (i=0; i<indent; i++)
        printf( "   " );
}
void c_cyc_object::display_links( int indent )
{
    int i;
    class c_cyc_object *ptr;
    char buffer[256];

    for (ptr=this; ptr; ptr=ptr->next_in_list )
    {

        display_indent( indent );
        printf("%s\n", ptr->co_textual_type?ptr->co_textual_type:"<NULL>" );
        for (i=0; i<ptr->co_next_link; i++)
        {
            display_indent( indent );
            printf("+-%s\n", ptr->co_links[i].string );
            switch (ptr->co_links[i].link_type)
            {
            case co_link_type_object:
                if (ptr->co_links[i].data.object)
                {
                    ptr->co_links[i].data.object->display_links( indent+1 );
                }
                else
                {
                    display_indent( indent+1 );
                    printf("<Null object>\n");
                }
                break;
            case co_link_type_string:
                display_indent( indent+1 );
                printf("%s\n", lex_string_from_terminal(ptr->co_links[i].data.string) );
                break;
            case co_link_type_symbol:
                display_indent( indent+1 );
                printf("%s\n", lex_string_from_terminal(ptr->co_links[i].data.symbol) );
                break;
            case co_link_type_sized_int:
                display_indent( indent+1 );
                lex_string_from_terminal( ptr->co_links[i].data.sized_int, buffer, 256 );
                printf( "%s\n", buffer );
                break;
            }
        }
    }
}

/*f c_cyc_object::get_number_of_links
 */
int c_cyc_object::get_number_of_links( void )
{
    return co_next_link;
}

/*f c_cyc_object::get_data
 */
int c_cyc_object::get_data( const char **co_textual_type, int *co_type, char *handle, char *parent_handle, t_lex_file_posn **start, t_lex_file_posn **end )
{
    *co_textual_type = this->co_textual_type;
    *co_type = (int)this->co_type;
    *start = &(this->start_posn);
    *end = &(this->end_posn);
    if (!next_in_list)
    {
        *handle = 0;
    }
    else
    {
        next_in_list->get_handle(handle);
    }
    if (!parent)
    {
        *parent_handle = 0;
    }
    else
    {
        parent->get_handle(parent_handle);
    }
    return 1;
}

/*f c_cyc_object::get_link
 */
int c_cyc_object::get_link( int link, const char **text, char *handle, const char **link_text, int *stage, int *type )
{
    if ((link<0) || (link>=co_next_link))
        return 0;

    *text = co_links[link].string;
    *handle = 0;
    *link_text = NULL;
    switch (co_links[link].link_type)
    {
    case co_link_type_object:
        if (co_links[link].data.object)
            co_links[link].data.object->get_handle(handle);
        break;
    case co_link_type_string:
        *link_text = lex_string_from_terminal( co_links[link].data.string );
        break;
    case co_link_type_symbol:
        *link_text = lex_string_from_terminal( co_links[link].data.string );
        break;
    default:
        break;
    }
    *stage = (int)co_links[link].compile_stage;
    *type = (int)co_links[link].link_type;
    return 1;
}

/*f get_lex_file_posn
 */
t_lex_file_posn c_cyc_object::get_lex_file_posn( int end_not_start )
{
    if (end_not_start)
        return end_posn;
    return start_posn;
}

/*a Seeking
 */
/*f c_cyc_object::find_object_from_file_posn
 */
c_cyc_object *c_cyc_object::find_object_from_file_posn( struct t_lex_file *lex_file, int file_char_position )
{
    c_cyc_object *co;
    for (co = this; co; co=co->next_in_list)
    {
        if ( (lex_char_relative_to_posn( &co->start_posn, 0, lex_file, file_char_position )<0) &&
             (lex_char_relative_to_posn( &co->end_posn, 1, lex_file, file_char_position )>0) )
        {
            return co;
        }
    }
    return NULL;
}



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


