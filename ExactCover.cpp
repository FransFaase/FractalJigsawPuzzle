/*  ExactCover - solving exact cover problems

    On the stdin it expects the representation of the exact cover, where
    each line on the input represents one vector. All vectors should be
    of the same length. The maximum length is NR_POSITIONS. A vector is
    represented by a sequence of 0 and 1. The remainder of the line (after
    skipping spaces) is used as the name for the vector. The name may be
    empty.

    In the file "reduced.ec" the reduced Exact Cover is written, which
    is made by applying logical reduction rules.
    The solutions are written to the file "ec_sols.txt" with one solution
    per line, where each line contains the names of the selected vectors
    terminated with a '|' character. Empty names are omitted.
    
    For more information see: http://www.iwriteiam.nl/Dpuzzle.html#EC
    
    The implementation makes use of dancing links, a technique suggested
    by D.E. Knuth. See: http://en.wikipedia.org/wiki/Dancing_Links
    
    Copyright (C) 2016 Frans Faase    http://www.iwriteiam.nl/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

GNU General Public License:
    http://www.iwriteiam.nl/GNU.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <time.h>

bool opt_reduce = true;
int opt_reduce_tries = 0;
bool opt_only_reduce = false;
bool opt_reduce_groups = false;

bool tracePrint()
{
    return false;
    
    static clock_t tick = clock() + 5000;
    if (clock() < tick)
        return false;
        
    tick = clock() + 5000;
    return true;
}



#define NR_POSITIONS 4000

struct Vector;
struct Position;
struct PositionConnection;

struct Node
{
    Node() : l(this), r(this), d(this), u(this), vector(0), position(0), next_ignored(0), swapped_out(false) {}
    void push_back(Node* n)   { n->r = this, n->l = l; l->r = n; l = n; }
    void push_bottom(Node* n) { n->d = this, n->u = u; u->d = n; u = n; }
    void swapout_horz() { l->r = r; r->l = l; swapped_out = true; }
    void swapin_horz() { l->r = this; r->l = this; swapped_out = false; }
    void swapout_vert() { u->d = d; d->u = u; swapped_out = true; }
    void swapin_vert() { u->d = this; d->u = this; swapped_out = false; }
    Node *l;
    Node *r;
    Node *d;
    Node *u;
    Vector *vector;
    Position *position;
    Node* next_ignored;
    virtual void ignore(bool mark_as_hot) {}
    virtual void unignore() {}
    
    // debugging
    bool swapped_out;
};

struct Vector : public Node
{
    Vector(long n) : nr(n), hot(1) {}
    char *name;
    long nr;
    long hot;
    virtual void ignore(bool mark_as_hot);
    virtual void unignore();
};

struct Position : public Node
{
    int nr;
    int nr_vec_left;
    long hotpos;
    long needs_reducing;
    PositionConnection* connections;
    virtual void ignore(bool mark_as_hot);
    virtual void unignore();
};

struct PositionConnection
{
    long nr;
    Position* from_pos;
    Position* to_pos;
    PositionConnection* next_con;
    PositionConnection* next_from_con;
    PositionConnection* next_to_con;
    bool enabled;
    Position* other(Position* p) { return p == from_pos ? to_pos : from_pos; }
    PositionConnection* next(Position* p) { return p == 0 ? next_con : p == from_pos ? next_from_con : next_to_con; }
    PositionConnection** ref_to_next(Position* p) { return p == 0 ? &next_con : p == from_pos ? &next_from_con : &next_to_con; }
};

Node root;
int nr_pos = 0;
int nr_vec = 0;
int nr_pos_with_zero_vec = 0;

void read(FILE *f)
{
    char buf[2*NR_POSITIONS];
    
    while (fgets(buf, 2*NR_POSITIONS, f))
    {
        if (nr_pos == 0)
        {
            for (int i = 0; i < NR_POSITIONS && (buf[i] == '0' || buf[i] == '1'); i++)
                nr_pos++;
            
            nr_pos_with_zero_vec = nr_pos;
            
            for (int i = 0; i < nr_pos; i++)
            {
                Position *position = new Position();
                root.push_back(position);
                position->nr = i;
                position->nr_vec_left = 0;
            }
        }
        
        
        nr_vec++;
        Vector *vector = new Vector(nr_vec);
        root.push_bottom(vector);
        
        char *s = buf;

        Position *position = (Position*)root.r;
        for (int i = 0; i < NR_POSITIONS && (*s == '0' || *s == '1'); i++, s++)
        {
            if (*s == '1')
            {
                Node *node = new Node();
                position->push_bottom(node);
                vector->push_back(node);
                node->vector = vector;
                node->position = position;
                if (position->nr_vec_left++ == 0)
                    nr_pos_with_zero_vec--;
            }
            position = (Position*)position->r;
        }
        while (*s == ' ')
            s++;
        int l = strlen(s);
        while (s[l-1] < ' ')
            s[--l] = 0;
        vector->name = (char*)malloc(sizeof(char)*(l+1));
        strcpy(vector->name, s);
    }
}

void read_numeric(FILE *f)
{
    {        
        Position *position = new Position();
        root.push_back(position);
        position->nr = nr_pos;
        position->nr_vec_left = 0;
        nr_pos++;
        nr_pos_with_zero_vec++;
    }

    char buf[2*NR_POSITIONS];
    while (fgets(buf, 2*NR_POSITIONS, f))
    {
        char *s = buf;
        
        if (!isdigit(*s))
            break;
            
        nr_vec++;
        Vector *vector = new Vector(nr_vec);
        root.push_bottom(vector);
        Position *position = (Position*)root.r;

        while (isdigit(*s))
        {
            int pos_nr = 0;
            while (isdigit(*s))
                pos_nr = 10 * pos_nr + *s++ - '0';
            if (*s == ',')
                s++;
            
            for (; pos_nr >= nr_pos; nr_pos++)
            {
                Position *position = new Position();
                root.push_back(position);
                position->nr = nr_pos;
                position->nr_vec_left = 0;
                nr_pos_with_zero_vec++;
            }
            
            while (position->nr < pos_nr)
                position = (Position*)position->r;

            Node *node = new Node();
            position->push_bottom(node);
            vector->push_back(node);
            node->vector = vector;
            node->position = position;
            if (position->nr_vec_left++ == 0)
                nr_pos_with_zero_vec--;
        }
        
        while (*s == ' ')
            s++;
        int l = strlen(s);
        while (s[l-1] < ' ')
            s[--l] = 0;
        vector->name = (char*)malloc(sizeof(char)*(l+1));
        strcpy(vector->name, s);
    }
}

void print(FILE *f)
{
    for (Vector *vector = (Vector*)root.d; vector != &root; vector = (Vector*)vector->d)
    {
        Node *node = vector->r;
        
        for (Position *position = (Position*)root.r; position != &root; position = (Position*)position->r)
        {
            if (node != vector && node->position == position)
            {
                fprintf(f, "1");
                node = node->r;
            }
            else
                fprintf(f, "0");
        }
        if (vector->name[0] != '\0')
            fprintf(f, " %s\n", vector->name);
        else
            fprintf(f, "\n", vector->nr);
    }
}

void print_numeric(FILE *f)
{
    for (Vector *vector = (Vector*)root.d; vector != &root; vector = (Vector*)vector->d)
    {
        Node *node = vector->r;
        
        int pos_nr = 0;
        bool first = true;
        for (Position *position = (Position*)root.r; position != &root; position = (Position*)position->r, pos_nr++)
        {
            if (node != vector && node->position == position)
            {
                if (!first)
                    fprintf(f, ",");
                first = false;
                fprintf(f, "%d", pos_nr);
                node = node->r;
            }
        }
        if (vector->name[0] != '\0')
            fprintf(f, " %s\n", vector->name);
        else
            fprintf(f, "\n", vector->nr);
    }
}

FILE *flog = 0;

void backup()
{
    static clock_t tt = clock();
    static int c = 0;
    if (clock() - tt > 300000)
    {
        const char *backupfilename = c == 0 ? "backup1.ec" : "backup2.ec";
        c = (c+1)%2;
        if (flog != 0) fprintf(flog, "Start creating %s\n", backupfilename);
        FILE* f = fopen(backupfilename, "wt");
        print_numeric(f);
        fclose(f);
        if (flog != 0) fprintf(flog, "Finished creating %s\n", backupfilename);
        tt = clock();
    }
}

void ignoreVector(Vector* vector, Position* exclude, bool mark_as_hot = false)
{
    vector->swapout_vert();
    
    for (Node* node = vector->r; node != vector; node = node->r)
    {
        if (mark_as_hot)
        {
            if (node->position->hotpos == 0)
                for (Node* node2 = node->position->d; node2 != node->position; node2 = node2->d)
                    node2->vector->hot++;
            node->position->hotpos++;
            node->position->needs_reducing++;
        }
        if (node->position != exclude)
        {
            node->swapout_vert();
            //node->position->nr_vec_left--;
            node->position->needs_reducing++;
            if (--node->position->nr_vec_left == 0)
                nr_pos_with_zero_vec++;
        }
    }
}

void unignoreVector(Vector* vector, Position* exclude)
{
    for (Node* node = vector->l; node != vector; node = node->l)
    {
        if (node->position != exclude)
        {
            node->swapin_vert();
            //node->position->nr_vec_left++;
            if (node->position->nr_vec_left++ == 0)
                nr_pos_with_zero_vec--;
        }
    }

    vector->swapin_vert();
    //vector->hot = false;
}

void selectPosition(Position* position, Vector* exclude)
{
    position->swapout_horz();
    
    for (Node* node = position->d; node != position; node = node->d)
    {
        if (node->vector != exclude)
            ignoreVector(node->vector, position);
    }
}

void unselectPosition(Position* position, Vector* exclude)
{
    for (Node* node = position->u; node != position; node = node->u)
    {
        if (node->vector != exclude)
            unignoreVector(node->vector, position);
    }

    position->swapin_horz();
}

void selectVector(Vector* vector)
{
    vector->swapout_vert();
    
    for (Node* node = vector->r; node != vector; node = node->r)
        selectPosition(node->position, vector);
}

void unselectVector(Vector* vector)
{
    for (Node* node = vector->l; node != vector; node = node->l)
        unselectPosition(node->position, vector);

    vector->swapin_vert();
    //vector->hot = false;
}

void ignorePosition(Position* position, bool mark_as_hot = false)
{
    if (position->nr_vec_left == 0)
        nr_pos_with_zero_vec--;
        
    position->swapout_horz();
    
    for (Node* node = position->d; node != position; node = node->d)
    {
        node->swapout_horz();
    }
}

void unignorePosition(Position* position)
{
    for (Node* node = position->u; node != position; node = node->u)
        node->swapin_horz();

    position->swapin_horz();

    if (position->nr_vec_left == 0)
        nr_pos_with_zero_vec++;
}

void Vector::ignore(bool mark_as_hot = false)
{
    ignoreVector(this, 0, mark_as_hot);
    nr_vec--;
}

void Vector::unignore()
{
    unignoreVector(this, 0);
    nr_vec++;
}

void Position::ignore(bool mark_as_hot = false)
{
    ignorePosition(this, mark_as_hot);
    nr_pos--;
}

void Position::unignore()
{
    unignorePosition(this);
    nr_pos++;
}

class IgnoredNodes
{
public:
    IgnoredNodes() : _ignored(0) {}
    
    void add(Node* node, bool mark_as_hot = false)
    {
        node->next_ignored = _ignored;
        _ignored = node;
        
        node->ignore(mark_as_hot);
    }
    
    ~IgnoredNodes()
    {
        while (_ignored != 0)
        {
            Node* next_ignored = _ignored->next_ignored;
            _ignored->next_ignored = 0;
            _ignored->unignore();
            _ignored = next_ignored;
        }
    }

private:
    Node* _ignored;
};


bool reduce(Position* position1, IgnoredNodes &ignoredNodes, bool mark_as_hot = false)
{
    bool progress = false;
    
    if (tracePrint())
        fprintf(stderr, "Processing position %d\n", position1->nr);
    
    for (Position *position2 = (Position*)root.r; position2 != &root; position2 = (Position*)position2->r)
    {
        if (position1->swapped_out)
        {
            fprintf(stderr, "Fatal error: Position1 %d swapped out\n", position1->nr);
            exit(1);
        }
        if (position2->swapped_out)
        {
            fprintf(stderr, "Fatal error: Position2 %d swapped out\n", position2->nr);
            exit(1);
        }
        
        if (position1 != position2 && position1->nr_vec_left == position2->nr_vec_left)
        {
            Node* node1 = position1->d;
            Node* node2 = position2->d;
            while (   node1 != position1 
                   && node2 != position2
                   && node1->vector == node2->vector)
            {
                node1 = node1->d;
                node2 = node2->d;
            }
            
            if (node1 == position1 && node2 == position2)
            {
                if (flog != 0) fprintf(flog, "Column %d equal with column %d. (%d)\n", position1->nr, position2->nr, nr_pos-1);
                
                ignoredNodes.add(position2);
            }

            if (nr_pos_with_zero_vec > 0)
            {
                fprintf(stderr, "Fatal error: Equal resulted in position(s) without vectors.\n");
                for (Position *position3 = (Position*)root.r; position3 != &root; position3 = (Position*)position3->r)
                    if (position3->nr_vec_left == 0)
                        fprintf(stderr, "Position %d has no vectors left\n", position3->nr);
                // Columns 53587 equal 53588:                leeg: 53566,        53570,       53575,         53580
                
                // 53587 implies 53588. 3x removed "row 89:" leeg: 53566, 53569, 53570, 53574, 53575, 53579, 53580
                exit(1);
            }
        }
    }

    for (Position *position2 = (Position*)root.r; position2 != &root; position2 = (Position*)position2->r)
    {
        if (position1->swapped_out)
        {
            fprintf(stderr, "Fatal error: Position1 %d swapped out\n", position1->nr);
            exit(1);
        }
        if (position2->swapped_out)
        {
            fprintf(stderr, "Fatal error: Position2 %d swapped out\n", position2->nr);
            exit(1);
        }
        
        if (position1->nr_vec_left < position2->nr_vec_left)
        {
            if (position1->nr_vec_left == 0)
            {
                fprintf(stderr, "Impossible\n");
                return progress; // -- no solution possible
            }
        
            //printf("test %d (%d) => %d (%d)\n", position1->nr, position1->nr_vec_left, position2->nr, position2->nr_vec_left);

            bool implied = true;

            Node* node2 = position2->d;
            for (Node* node1 = position1->d; node1 != position1; node1 = node1->d)
            {
                int vec_nr = node1->vector->nr;
                
                while (node2 != position2 && node2->vector->nr < vec_nr)
                    node2 = node2->d;
                    
                if (node1->vector != node2->vector)
                {
                    implied = false;
                    break;
                }
            }

            //printf("Implied = %d\n", implied);
            
            if (implied)
            {
                if (flog != 0) fprintf(flog, "Column %d implies column %d. Reduced number vectors with %d\n", position1->nr, position2->nr, position2->nr_vec_left - position1->nr_vec_left);  

                Vector *next_vector = 0;
                for (Vector *vector = (Vector*)root.d; vector != &root; vector = next_vector)
                {
                    next_vector = (Vector*)vector->d;
                    
                    bool has_position1 = false;
                    bool has_position2 = false;
                    
                    for (Node* node = vector->r; node != vector; node = node->r)
                    {
                        if (node->position == position1)
                            has_position1 = true;
                        if (node->position == position2)
                            has_position2 = true;
                    }
                    
                    if (!has_position1 && has_position2)
                    {
                        if (flog != 0) fprintf(flog, "  remove: %s\n", vector->name);

                        ignoredNodes.add(vector, mark_as_hot);
                    }
                }
                
                if (flog != 0) fprintf(flog, "Left %d.\n", nr_vec);
                
                progress = true;

                if (nr_pos_with_zero_vec > 0)
                {
                    if (flog != 0) fprintf(flog, "Reduction caused some positions to have no vectors anymore.\n");
                    for (Position *position3 = (Position*)root.r; position3 != &root; position3 = (Position*)position3->r)
                        if (position3->nr_vec_left == 0)
                        {
                            if (flog != 0) fprintf(flog, "  Removed position %d\n", position3->nr);
                            ignoredNodes.add(position3);
                        }
                        
                    if (nr_pos_with_zero_vec > 0)
                    {
                        fprintf(stderr, "Fatal error: Still some left???\n");
                        exit(1);
                    }
                    // Columns 53587 equal 53588:                leeg: 53566,        53570,       53575,         53580
                    
                    // 53587 implies 53588. 3x removed "row 89:" leeg: 53566, 53569, 53570, 53574, 53575, 53579, 53580
                    //exit(1);
                }
            }
        }
    }
    
    backup();
    
    return progress;
}

void insert_connection(PositionConnection** cons, PositionConnection* con, Position* pos)
{
    while ((*cons) != 0 && (*cons)->nr >= con->nr)
        cons = (*cons)->ref_to_next(pos);
        
    *con->ref_to_next(pos) = (*cons);
    *cons = con;
}
    
bool reduce_groups(IgnoredNodes &ignoredNodes, bool mark_as_hot)
{
    bool progress = false;

    if (flog != 0) fprintf(flog, "Start reduce groups\n");
    
    PositionConnection* all_pos_connections = 0;
    
    for (Position *position = (Position*)root.r; position != &root; position = (Position*)position->r)
        position->connections = 0;
    
    for (Position *position1 = (Position*)root.r; position1 != &root; position1 = (Position*)position1->r)
        for (Position *position2 = (Position*)position1->r; position2 != &root; position2 = (Position*)position2->r)
        {
            Node* node1 = position1->d;
            Node* node2 = position2->d;
            
            long nr_common = 0;
            
            while (node1 != position1 && node2 != position2)
            {
                int vec_nr1 = node1->vector->nr;
                int vec_nr2 = node2->vector->nr;
                
                if (vec_nr1 < vec_nr2)
                    node1 = node1->d;
                else if (vec_nr2 < vec_nr1)
                    node2 = node2->d;
                else
                {
                    nr_common++;
                    node1 = node1->d;
                    node2 = node2->d;
                }
            }
            
            if (nr_common > 0)
            {
                PositionConnection* new_pos_con = new PositionConnection;
                new_pos_con->nr = nr_common;
                new_pos_con->from_pos = position1;
                new_pos_con->to_pos = position2;
                //printf("found %d from %d (%d) to %d (%d)\n", new_pos_con->nr, 
                //        new_pos_con->from_pos->nr, new_pos_con->from_pos->nr_vec_left,
                //        new_pos_con->to_pos->nr, new_pos_con->to_pos->nr_vec_left);
                
                insert_connection(&all_pos_connections, new_pos_con, 0);
                insert_connection(&position1->connections, new_pos_con, position1);
                insert_connection(&position2->connections, new_pos_con, position2);
            }
                
        }
        
#define MAX_GROUP_SIZE 12
#define MAX_GROUP_SIZE2 (1<<MAX_GROUP_SIZE)
    
    for (int group_size = 3; group_size <= MAX_GROUP_SIZE && !progress; group_size++)
    {
        if (flog != 0) fprintf(flog, "group size = %d\n", group_size);
        
        int group_size2 = 1 << group_size;
        
        long nr_pos_con = 0;
        for (PositionConnection* pos_con = all_pos_connections; pos_con != 0; pos_con = pos_con->next_con)
        {
            pos_con->enabled = true;
            nr_pos_con++;
        }
        
        for (int gr = 0; gr < nr_pos_con/3; gr++)
        {
            //printf("Group %d\n", gr);
            
            int nr_con;
            
            Position* positions[MAX_GROUP_SIZE];
            
            for (PositionConnection* pos_con = all_pos_connections; pos_con != 0; pos_con = pos_con->next_con)
                if (pos_con->enabled)
                {
                    positions[0] = pos_con->from_pos;
                    positions[1] = pos_con->to_pos;
                    nr_con = pos_con->nr;
                    pos_con->enabled = false;
                    break;
                }

            bool correct = true;
            
            for (int i = 2; i < group_size; i++)
            {
                //printf("  add %d\n", i);
                
                Position* best_pos = 0;
                long max_nr = 0;
                for (int j = 0; j < i; j++)
                {
                    for (PositionConnection* pos_con = positions[j]->connections; pos_con != 0; pos_con = pos_con->next(positions[j]))
                        if (pos_con->enabled)
                        {
                            if (best_pos == 0 || max_nr < pos_con->nr)
                            {
                                Position* new_pos = pos_con->other(positions[j]);
                                
                                for (int k = 0; k < i; k++)
                                    if (positions[k] == new_pos)
                                        new_pos = 0;
                                if (new_pos != 0)
                                {
                                    best_pos = new_pos;
                                    max_nr = pos_con->nr;
                                }
                            }
                            break;
                        }
                }
                
                if (best_pos == 0)
                {
                    //printf("incorrect\n");
                    correct = false;
                    break;
                }
                
                positions[i] = best_pos;
                nr_con += max_nr;
            }
            
            if (correct)
            {
                if (flog != 0)
                {
                    fprintf(flog, "group %d: ", gr);
                    for (int i = 0; i < group_size; i++)
                        fprintf(flog, " %d", positions[i]->nr);
                    fprintf(flog, " = %d. ", nr_con);
                }
                                
                long count[MAX_GROUP_SIZE2];
                bool possible[MAX_GROUP_SIZE2];
                
                for (int i = 0; i < group_size2; i++)
                {
                    count[i] = 0;
                    possible[i] = i == 0;
                }
                int nr_possible = 1;
            
                Node* nodes[MAX_GROUP_SIZE];
                for (int i = 0; i < group_size; i++)
                    nodes[i] = positions[i]->d;
                    
                for (;;)
                {
                    int min_vec_nr = -1;
    
                    for (int i = 0; i < group_size; i++)
                        if (nodes[i] != positions[i] && (min_vec_nr == -1 || nodes[i]->vector->nr < min_vec_nr))
                            min_vec_nr = nodes[i]->vector->nr;
                            
                    if (min_vec_nr == -1)
                        break;
                    
                    int val = 0;
                    for (int i = 0; i < group_size; i++)
                        if (nodes[i] != positions[i] && nodes[i]->vector->nr == min_vec_nr)
                        {
                            val |= 1 << i;
                            nodes[i] = nodes[i]->d;
                        }
                        
                    count[val]++;
                    if (count[val] == 1)
                    {
                        possible[val] = true;
                        //nr_possible++;
                        
                        for (int val2 = 1; val2 < group_size2; val2++)
                            if (possible[val2] && (val & val2) == 0)
                                possible[val|val2] = true;
                        
                        //if (nr_possible == group_size2)
                        //    break;
                    }
                }

                for (int i = 1; i < group_size2; i++)
                {
                    //for (int j = 0; j < group_size; j++)
                    //    printf("%c", (i & (1 << j)) ? '1' : '0');
                    //printf(": %d %s ", count[i], possible[i] ? "t" : "f");
                    if (possible[i])
                        nr_possible++;
                }

                if (flog != 0) fprintf(flog, " nr possible = %d\n", nr_possible);
                
                bool something_to_reduce = false;
                bool to_be_reduced[MAX_GROUP_SIZE2];
                
                for (int i = 1; i < group_size2; i++)
                    if (to_be_reduced[i] = (count[i] > 0 && !possible[group_size2 - 1 - i]))
                    {
                        if (flog != 0)
                        {
                            fprintf(flog, "- reduce %d for:", count[i]);
                            for (int j = 0; j < group_size; j++)
                                fprintf(flog, "%c", (i & (1 << j)) ? '1' : '0');
                            fprintf(flog, "\n");
                        }
                                                
                        something_to_reduce = true;
                    }
                    
                if (something_to_reduce)
                {
                    for (int i = 0; i < group_size; i++)
                        nodes[i] = positions[i]->d;
                        
                    for (;;)
                    {
                        Vector* min_vec = 0;
                        int min_vec_nr = -1;
        
                        for (int i = 0; i < group_size; i++)
                            if (nodes[i] != positions[i] && (min_vec_nr == -1 || nodes[i]->vector->nr < min_vec_nr))
                            {
                                min_vec = nodes[i]->vector;
                                min_vec_nr = min_vec->nr;
                            }
                                
                        if (min_vec == 0)
                            break;
                        
                        int val = 0;
                        for (int i = 0; i < group_size; i++)
                            if (nodes[i] != positions[i] && nodes[i]->vector->nr == min_vec_nr)
                            {
                                val |= 1 << i;
                                nodes[i] = nodes[i]->d;
                            }
                            
                        if (to_be_reduced[val])
                        {
                            if (flog != 0) fprintf(flog, "  remove: %s\n", min_vec->name);
                            ignoredNodes.add(min_vec, mark_as_hot);
                            progress = true;
                        }
                    }
                }
            }            
        }
    }
    
    return progress;
}

void reduce(IgnoredNodes &ignoredNodes, bool mark_as_hot = false)
{
    for (bool progress = true; progress;)
    {
        progress = false;
        bool reducing_groups_useful = false;

        for (;;)
        {
            Position *position_to_reduce = 0;
            long score;
            for (Position *position1 = (Position*)root.r; position1 != &root; position1 = (Position*)position1->r)
                if (position1->needs_reducing > 0 && (position_to_reduce == 0 || position1->nr_vec_left - position1->needs_reducing < score))
                {
                    position_to_reduce = position1;
                    score = position1->nr_vec_left - position1->needs_reducing;
                }
        
            if (position_to_reduce == 0)
                break;
            
            if (reduce(position_to_reduce, ignoredNodes, mark_as_hot))
                reducing_groups_useful = true;
            position_to_reduce->needs_reducing = 0;
        }
                    
        if (reducing_groups_useful && opt_reduce_groups)
            progress = reduce_groups(ignoredNodes, mark_as_hot);
            
    }
    if (flog != 0) fprintf(flog, "done reducing\n");    
}

long total_nr_calls = 0;
int nr_calls;

bool possible(int tries)
{
    if (tries == 0)
        return true;

    nr_calls++;
    total_nr_calls++;        
    if (nr_pos_with_zero_vec > 0)
        return false;
        
    int min_nr_vec_left = 0;
    Position* sel_pos = 0;
    
    for (Position *position = (Position*)root.r; position != &root; position = (Position*)position->r)
        if (position->nr_vec_left < min_nr_vec_left || sel_pos == 0)
        {
            sel_pos = position;
            min_nr_vec_left = position->nr_vec_left;
            if (min_nr_vec_left == 1)
                break;
        }
    if (min_nr_vec_left == 0)
        return true;
        
    tries /= min_nr_vec_left;
    if (tries == 0)
        return true;

    IgnoredNodes ignoredNodes;
        
    for (Node *node = sel_pos->d; node != sel_pos; node = node->d)
    {
        Vector *vector = node->vector;

        selectVector(vector);
        
        bool pos = possible(tries);
        
        unselectVector(vector);

        if (pos)
            return true;
    }
        
    return false;
}


clock_t start_time;
clock_t start_periode;
int sol_found_in_periode;

int nr_solutions = 0;

Vector* sol_vectors[NR_POSITIONS];
int nr_sol_vectors = 0;

FILE* fsols;

long nr_calls_to_solve = 0;

bool solve()
{
    nr_calls_to_solve++;
    
    // Found solution if there are no positions left
    if (root.r == &root)
    {
        nr_solutions++;
        
        //if (nr_solutions < 100)
        //{
        for (int i = 0; i < nr_sol_vectors; i++)
            if (*sol_vectors[i]->name != '\0')
                fprintf(fsols, "%s|", sol_vectors[i]->name);
        fprintf(fsols, "\n");
        //}
        
        clock_t now = clock();
        if (now > start_periode + 1000)
        {
            if (flog != 0)
            {
                fprintf(flog, "%4d: ", (start_periode - start_time)/1000);
                for (; sol_found_in_periode > 0; sol_found_in_periode--)
                    fprintf(flog, "*");
                fprintf(flog, "  %lf\n", nr_solutions / ((now - start_time)/1000.0));
            }
            start_periode += 1000;
        }
        while (now > start_periode + 1000)
        {
            if (flog != 0) fprintf(flog, "%4d: \n", (start_periode - start_time)/1000);
            start_periode += 1000;
        }
        
        sol_found_in_periode++;
        
        return false; // true: stop searching
    }

    if (tracePrint())
    {
        fprintf(stderr, "working on:\n");
        for (int i = 0; i < nr_sol_vectors; i++)
            fprintf(stderr, "  %s\n", sol_vectors[i]->name);
    }
        
    IgnoredNodes ignoredNodes;
    for(;;)
    {
        // If there is a position that cannot be filled, then stop
        if (nr_pos_with_zero_vec > 0)
        {
            //printf("%*.*simpossible\n", nr_sol_vectors, nr_sol_vectors, "");
            return false;
        }
        
        Position* best_pos = 0;
        int best_nr = 0;
        for (Position *position = (Position*)root.r; position != &root; position = (Position*)position->r)
        {
            if (position->nr_vec_left == 1)
            {
                best_pos = position;
                best_nr = 1;
                break;
            }
            
            if (best_pos == 0 || position->nr_vec_left > best_nr)
            {
                best_pos = position;
                best_nr = position->nr_vec_left;
            }
        }

#if 0 // use reduction while solving
        int nr_hot = 0;
        int nr_impos = 0;
        int nr_pass = 0;
    
        for (bool changes = true; changes; )
        {
            nr_pass++;
            changes = false;
            
            for (Vector *vector = (Vector*)root.d; vector != &root; vector = (Vector*)vector->d)
            {
                if (vector->hot)
                {
                    nr_hot++;
                    selectVector(vector);
                    
                    // Evaluate solution
                    bool pos = possible(1);
    
                    unselectVector(vector);
                    
                    if (!pos)
                    {
                        nr_impos++;
                        if (flog != 0) fprintf(flog, "%*.*sremoved impossible %s\n", nr_sol_vectors, nr_sol_vectors, "", vector->name);
                        ignoredNodes.add(vector);
                        changes = true;
                        vector->hot = false;
                    }
                    vector->hot = false;
                }
            }
        }
        //printf("%*.*s%d:%d %d\n", nr_sol_vectors, nr_sol_vectors, "", nr_hot, nr_impos, nr_pass);
#endif    


        //printf("%*.*sbest = %d\n", nr_sol_vectors, nr_sol_vectors, "", best_nr);

        if (best_nr == 0)
        {
            fprintf(stderr, "ERROR\n");
            return false;
        }
            
#if 0        
        if (best_nr > 500)
        {
            // Select the vector whoes positions have the lowest number of vectors
            best_vec = 0;
            best_nr = 0;
            for (Node *node_of_best = best_pos->d; node_of_best != best_pos; node_of_best = node_of_best->d)
            {
                Vector *vector = node_of_best->vector;
            
                int nr = 0;
                for (Node* node = vector->r; node != vector; node = node->r)
                    nr += node->position->nr_vec_left;
                
                if (best_vec == 0 || nr < best_nr)
                {
                    best_vec = vector;
                    best_nr = nr;
                }
            }
        }
#endif
        
        Vector* sel_vector = best_pos->d->vector;
        sol_vectors[nr_sol_vectors++] = sel_vector;
        selectVector(sel_vector);
    
        bool result = solve();
    
        unselectVector(sel_vector);
        nr_sol_vectors--;
        
        if (result)
            return true;

        if (best_nr == 1)
            return false;
            
        ignoredNodes.add(sel_vector);

    }        
    
    return false;
}

int main(int argc, char* argv[])
{
    bool use_numeric_input_format = false;
    bool output_intermediate_reduce_results = false;
    
    for (int i = 1; i < argc; i++)
    {
        char* arg = argv[i];
        if (strcmp(arg, "-noreduce") == 0 || strcmp(arg, "-nored") == 0)
            opt_reduce = false;
        else if (   (strcmp(arg, "-reducetries") == 0 || strcmp(arg, "-redtries") == 0)
                 && i+1 < argc)
        {
            opt_reduce_tries = atoi(argv[i+1]);
            i++;
        }
        else if (strcmp(arg, "-onlyreduce") == 0 || strcmp(arg, "-onlyred") == 0)
            opt_only_reduce = true;
        else if (strcmp(arg, "-reducegroups") == 0 || strcmp(arg, "-redgr") == 0)
            opt_reduce_groups = true;
        else if (strcmp(arg, "-numeric") == 0 || strcmp(arg, "-num") == 0)
            use_numeric_input_format = true;
        else if (strcmp(arg, "-save_intermediate") == 0)
            output_intermediate_reduce_results = true;
        else        
            fprintf(stderr, "Error: Unknown argument %s\n", arg);
    }
    
    if (use_numeric_input_format)
        read_numeric(stdin);
    else
        read(stdin);

    {
        bool impossible = false;
        for (Position *position = (Position*)root.r; position != &root; position = (Position*)position->r)
            if (position->nr_vec_left == 0)
            {
                impossible = true;
                fprintf(stderr, "Column %d is empty.\n", position->nr);
            }
        if (impossible)
        {
            fprintf(stderr, "Impossible Exact Cover.\n");
            return 0;
        }
    }
    
    flog = stderr;
    fsols = stdout; //fopen("ec_sols.txt", "wt");
    IgnoredNodes ignoredNodes;

    start_time = clock();
    start_periode = start_time;

    if (opt_reduce)
    {
        int nr_hot = 0;
        int nr_impos = 0;
        int nr_pass = 0;
        
        int nr_tries = opt_reduce_tries > 0 ? opt_reduce_tries : 1000;

        for (Position *position = (Position*)root.r; position != &root; position = (Position*)position->r)
            position->hotpos = 1;

        for (Position *position1 = (Position*)root.r; position1 != &root; position1 = (Position*)position1->r)
            position1->needs_reducing = 1;

        for (int changed = 1; changed > 0; )
        {
            reduce(ignoredNodes, true);
            
            nr_pass++;
            changed = 0;

            for (Vector *vector = (Vector*)root.d; vector != &root; vector = (Vector*)vector->d)
                vector->hot++;

            for (;;)
            {
            
                Vector *hottest_vector = 0;                
                for (Vector *vector = (Vector*)root.d; vector != &root; vector = (Vector*)vector->d)
                    if (vector->hot > 0 && (hottest_vector == 0 || vector->hot > hottest_vector->hot))
                        hottest_vector = vector;
                        
                if (hottest_vector == 0)
                    break;
                
                nr_hot++;
                selectVector(hottest_vector);
                
                // Evaluate solution
                bool pos = possible(1);

                unselectVector(hottest_vector);
                
                if (!pos)
                {
                    nr_impos++;
                    fprintf(stderr, "%*.*sremoved impossible %s\n", nr_sol_vectors, nr_sol_vectors, "", hottest_vector->name);
                    ignoredNodes.add(hottest_vector);
                    changed++;
                    hottest_vector->hot = 0;
                    //reduce(ignoredNodes, true);
                }
                hottest_vector->hot = 0;
            }
            
#if 0            
            for (Vector *vector = (Vector*)root.d; vector != &root; vector = (Vector*)vector->d)
            {
                if (tracePrint())
                    fprintf(stderr, "Trying to eliminate %s\n", vector->name);

                selectVector(vector);
                
                // Evaluate solution
                bool pos = possible(1);

                unselectVector(vector);
                
                if (!pos)
                {
                    nr_impos++;
                    fprintf(stderr, "%*.*seliminated %s [%d:%d] %d\n", nr_sol_vectors, nr_sol_vectors, "", vector->name, nr_vec, nr_hot, nr_calls);
                    ignoredNodes.add(vector, true);
                    reduce(ignoredNodes, true);
                    changed++;
                }
                
                backup();
            }
            
#endif

            if (changed == 0)
            {
                for (Vector *vector = (Vector*)root.d; vector != &root; vector = (Vector*)vector->d)
                {
                    nr_hot++;
                    vector->hot++;
                }
    
                for (;changed < 1000 || changed < nr_vec/10;)
                {
                    int min_score;
                    int nr_min_score = 0;
                    for (Position *position = (Position*)root.r; position != &root; position = (Position*)position->r)
                        if (position->hotpos > 0)
                        {
                            long score = position->nr_vec_left - position->hotpos;
                            if (score < min_score || nr_min_score == 0)
                            {
                                min_score = score;
                                nr_min_score = 1;
                            }
                            else if (score == min_score)
                                nr_min_score++;
                        }
                
                    if (nr_min_score == 0)
                        break;
                        
                    fprintf(stderr, "min score = %d, nr = %d [%d:%d]\n", min_score, nr_min_score, nr_vec, nr_hot);
    
                    for (Position *position = (Position*)root.r; position != &root; position = (Position*)position->r)
                    {
                        long score = position->nr_vec_left - position->hotpos;
                        if (score == min_score)
                        {
                            fprintf(stderr, "cold: %d\n", position->nr);
                            
                            bool some_removed = false;
                            
                            for (Node* node = position->d; node != position; node = node->d)
                            {
                                Vector* vector = node->vector;
                                if (vector->hot)
                                {
                                    vector->hot = false;
                                    nr_hot--;
                                    nr_calls = 0;
                                    selectVector(vector);
                                    
                                    // Evaluate solution
                                    fprintf(stderr, "possible = %d\n", nr_tries);
                                    bool pos = possible(nr_tries);
                    
                                    unselectVector(vector);
                                    
                                    if (!pos)
                                    {
                                        nr_impos++;
                                        fprintf(stderr, "%*.*sremoved impossible %s [%d:%d] %d\n", nr_sol_vectors, nr_sol_vectors, "", vector->name, nr_vec, nr_hot, nr_calls);
                                        /*for (Node* node = vector->r; node != vector; node = node->r)
                                        {
                                            if (node->position->hotpos == 0)
                                            {
                                                printf("hot %d\n", node->position->nr);
                                                for (Node* node2 = node->position->d; node2 != node->position; node2 = node2->d)
                                                    if (!node2->vector->hot)
                                                    {
                                                        node2->vector->hot = true;
                                                        nr_hot++;
                                                    }
                                            }
                                            node->position->hotpos++;
                                        }*/
                                        ignoredNodes.add(vector, true);
                                        changed++;
                                        some_removed = true;
                                        //if (nr_tries > 3)
                                        //    nr_tries -= 3;
                                    }
                                    else
                                    {
                                        nr_tries++;
                                        fprintf(stderr, "%d:%d ", nr_calls, nr_tries); fflush(stdout);
                                    }
                                    //else if (tracePrint())
                                    //    printf("%*.*spossible %s (%d)\n", nr_sol_vectors, nr_sol_vectors, "", vector->name, nr_hot);
                                }
                            }
                            position->hotpos = 0;
                            
                            if (some_removed)
                            {
                                fprintf(stderr, "Reduce:\n");
                                reduce(position, ignoredNodes, true);
                            }
                        }
                        
                        backup();
                    }
                }
            }
            
            if (changed > 0 && output_intermediate_reduce_results)
            {
                FILE *f = fopen("reduced.ec","wt");
                if (use_numeric_input_format)
                    print_numeric(f);
                else
                    print(f);
                fclose(f);
            }
        }
        fprintf(stderr, "%*.*s%d:%d %d\n", nr_sol_vectors, nr_sol_vectors, "", nr_hot, nr_impos, nr_pass);

        //print(stdout);
    }
    
    fprintf(stderr, "total nr of calls = %ld\n", total_nr_calls);
    
    if (opt_only_reduce)
        return 0;
            
    sol_found_in_periode = 0;

    solve();
    clock_t now = clock();
    while (now > start_periode + 1000)
    {
        fprintf(stderr, "%d\n", sol_found_in_periode);
        sol_found_in_periode = 0;
        start_periode += 1000;
    }
    fprintf(stderr, "total time = %lf\n", (now - start_time)/1000.0);
    fprintf(stderr, "nr solution = %d (%lf/sec)\n", nr_solutions, nr_solutions/((now - start_time)/1000.0));
    fprintf(stderr, "nr calls to solve = %ld\n", nr_calls_to_solve);
    //fclose(fsols);
}
