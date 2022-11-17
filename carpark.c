#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define GAP '.'
#define BOLLARD '#'
#define MIN_CAR_LENGTH 2
#define MIN_CAR_WIDTH 1
#define BORDER_WIDTH 1
#define BORDER_NUM_EACH_ORIENT 2
#define MIN_PARK_HEIGHT (MIN_CAR_LENGTH + BORDER_NUM_EACH_ORIENT * BORDER_WIDTH)
#define MIN_PARK_WIDTH (MIN_CAR_WIDTH + BORDER_NUM_EACH_ORIENT * BORDER_WIDTH)
#define PARK_SHORT_SIDE (MIN_PARK_HEIGHT > MIN_PARK_WIDTH ? MIN_PARK_WIDTH : MIN_PARK_HEIGHT)
#define PARK_LONG_SIDE (MIN_PARK_HEIGHT > MIN_PARK_WIDTH ? MIN_PARK_HEIGHT : MIN_PARK_WIDTH)
#define PARK_MAX_SIDE 20
// TODO: Check number
#define CAR_MAX 30
#define POSSIBILITIES 26000
#define FIRST_VEHICLE 'A'
#define NO_SOL_MSG "No Solution?"
#define PRINT_SOLUTION_FLG "-show"
#define BIG_STRING 500
#define MOVE_DIRECTIONS 2

typedef enum
{
  vertical,
  horizontal
} orientation;

typedef struct
{
  int head[2];
  orientation orient;
  bool is_exited;
  int length;
  int is_init;
} vehicle;

typedef struct
{
  int vehicles_num;
  vehicle vehicles[CAR_MAX];
  int origin;
  int steps;
} park;

typedef struct
{
  char p[PARK_MAX_SIDE][PARK_MAX_SIDE];
  int h;
  int w;
  bool has_solution;
  int possibilities;
  int best_park;
  int best_step;
  int cur_idx;
} initial_park;

bool validate_park(initial_park ip);
bool is_valid_inner_cell(char c);
bool is_valid_border_cell(char c);
bool get_vehicles_info(initial_park *ip, park *pk);
bool get_vehicle_info(initial_park *ip, park *pk, const int i, const int j);
int get_vehicle_lng(char p[PARK_MAX_SIDE][PARK_MAX_SIDE], int i, int j, orientation o);
bool is_finished_park(park p);
bool is_unique_park(park pks[POSSIBILITIES], int p_nums, park pk);
bool is_valid_move(initial_park ip, park pk, vehicle v, int v_i);
bool handle_at_exit(initial_park ip, park *pk, vehicle v, int v_i);
void get_options(initial_park *ip, park pks[POSSIBILITIES], int v_i);
int explore_parks(initial_park *ip, park pks[POSSIBILITIES]);
void print_park(char p[PARK_MAX_SIDE][PARK_MAX_SIDE], park pk, const int h, const int w);
void print_path(initial_park ip, park pks[POSSIBILITIES]);
bool string_to_brd(char p[PARK_MAX_SIDE][PARK_MAX_SIDE], initial_park *ip, const int h, const int w);
bool generate_vehicle(char p[PARK_MAX_SIDE][PARK_MAX_SIDE], vehicle v, char l);
bool move_by_orient(initial_park *ip, park pks[POSSIBILITIES], park *np, vehicle v, int v_i);
void test(void);

int main(int argc, char *argv[])
{
  if (argc < 2 || argc > 3)
  {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  bool is_print_solution = strcmp(argv[1], PRINT_SOLUTION_FLG) == 0;
  char *path = is_print_solution ? argv[2] : argv[1];

  FILE *fpin = fopen(path, "rb");
  if (!fpin)
  {
    fprintf(stderr, "Cannot read %s\n", path);
    exit(EXIT_FAILURE);
  }

  char str[PARK_MAX_SIDE];
  initial_park ip = {.possibilities = 1};
  // string_to_brd(validate_no_v, &ip24, 4, 4);

  if (fgets(str, PARK_MAX_SIDE, fpin) == NULL)
  {
    fprintf(stderr, "Cannot read width & height from %s\n", path);
    exit(EXIT_FAILURE);
  }

  if (sscanf(str, "%ix%i", &ip.h, &ip.w) < 0)
  {
    fprintf(stderr, "Cannot process the width & height received from %s\n", path);
    exit(EXIT_FAILURE);
  }

  int y = 0;
  while (fgets(ip.p[y], PARK_MAX_SIDE, fpin) != NULL)
  {
    y++;
  }

  park pk = {0};
  if (!validate_park(ip) || !get_vehicles_info(&ip, &pk))
  {
    exit(EXIT_FAILURE);
  }
  static park pks[POSSIBILITIES];
  pks[0] = pk;

  int steps = explore_parks(&ip, pks);
  if (steps == 0)
  {
    printf(NO_SOL_MSG);
  }
  else
  {
    printf("%i moves", steps);
  }

  test();
  return 0;
}

bool string_to_brd(char p[PARK_MAX_SIDE][PARK_MAX_SIDE], initial_park *ip, const int h, const int w)
{
  if (!ip || ip == NULL)
  {
    return false;
  }
  for (int i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++)
    {
      ip->p[i][j] = p[i][j];
    }
  }
  ip->h = h;
  ip->w = w;
  return true;
}

bool get_vehicles_info(initial_park *ip, park *pk)
{
  for (int i = BORDER_WIDTH; i < ip->h - BORDER_WIDTH; i++)
  {
    for (int j = BORDER_WIDTH; j < ip->w - BORDER_WIDTH; j++)
    {
      char c = ip->p[i][j];
      if (isalpha(c) > 0)
      {
        // duplicate vehicles
        if (pk->vehicles[c - FIRST_VEHICLE].is_init)
        {
          return false;
        }
        if (!get_vehicle_info(ip, pk, i, j))
        {
          return false;
        }
        pk->vehicles_num++;
      }
    }
  }
  // validate the park has at least one vehicle
  if (pk->vehicles_num == 0)
  {
    return false;
  }
  // validate vehicles start from A and in correct order
  for (int i = 0; i < pk->vehicles_num; i++)
  {
    if (!pk->vehicles[i].is_init)
    {
      return false;
    }
  }
  return true;
}

bool get_vehicle_info(initial_park *ip, park *pk, const int i, const int j)
{
  char c = ip->p[i][j];

  // validate orientation & minimum length
  bool is_vertical = c == ip->p[i + 1][j];
  bool is_horizontal = c == ip->p[i][j + 1];
  if (!is_vertical && !is_horizontal)
  {
    return false;
  }
  bool orientation = is_vertical ? vertical : horizontal;

  // validate length
  int lng = get_vehicle_lng(ip->p, i, j, orientation);
  if (lng < MIN_CAR_LENGTH)
  {
    return false;
  }

  // init vehicle
  pk->vehicles[c - FIRST_VEHICLE].head[0] = i;
  pk->vehicles[c - FIRST_VEHICLE].head[1] = j;
  pk->vehicles[c - FIRST_VEHICLE].is_init = true;
  pk->vehicles[c - FIRST_VEHICLE].is_exited = false;
  pk->vehicles[c - FIRST_VEHICLE].length = lng;
  pk->vehicles[c - FIRST_VEHICLE].orient = orientation;
  return true;
}

int get_vehicle_lng(char p[PARK_MAX_SIDE][PARK_MAX_SIDE], int i, int j, orientation o)
{
  if (!p || p == NULL)
  {
    return 0;
  }
  char letter = p[i][j];
  int lng = 0;
  while (p[i][j] == letter)
  {
    // un-mark for future validation
    p[i][j] = GAP;
    lng++;
    i = o ? i : i + 1;
    j = o ? j + 1 : j;
  }
  return lng;
}

bool is_unique_park(park pks[POSSIBILITIES], int p_nums, park pk)
{
  for (int i = 0; i < p_nums; i++)
  {
    bool is_unique = false;
    for (int j = 0; j < pk.vehicles_num; j++)
    {
      bool is_head_x_same = pk.vehicles[j].head[0] == pks[i].vehicles[j].head[0];
      bool is_head_y_same = pk.vehicles[j].head[1] == pks[i].vehicles[j].head[1];
      bool is_exit_same = pk.vehicles[j].is_exited == pks[i].vehicles[j].is_exited;

      if (!is_head_x_same || !is_head_y_same || !is_exit_same)
      {
        is_unique = true;
      }
    }
    if (!is_unique)
    {
      return false;
    }
  }
  return true;
}

int explore_parks(initial_park *ip, park pks[POSSIBILITIES])
{
  while (ip->cur_idx < ip->possibilities)
  {
    if (is_finished_park(pks[ip->cur_idx]))
    {
      // TODO: Refactor needed
      if (!ip->has_solution)
      {
        ip->best_step = pks[ip->cur_idx].steps;
        ip->best_park = ip->cur_idx;
        ip->has_solution = true;
      }
      else if (pks[ip->cur_idx].steps < ip->best_step)
      {
        ip->best_step = pks[ip->cur_idx].steps;
        ip->best_park = ip->cur_idx;
      }
    }
    for (int i = 0; i < pks[ip->cur_idx].vehicles_num; i++)
    {
      if (pks[ip->cur_idx].vehicles[i].is_exited == false)
      {
        get_options(ip, pks, i);
      }
    }
    ip->cur_idx++;
  }
  if (ip->has_solution)
  {
    print_path(*ip, pks);
  }
  return ip->has_solution ? ip->best_step : 0;
}

void print_park(char p[PARK_MAX_SIDE][PARK_MAX_SIDE], park pk, const int h, const int w)
{
  char p_n[PARK_MAX_SIDE][PARK_MAX_SIDE];

  for (int i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++)
    {
      p_n[i][j] = p[i][j];
    }
  }

  for (int i = 0; i < pk.vehicles_num; i++)
  {
    vehicle v = pk.vehicles[i];
    bool is_exited = v.is_exited;
    char letter = FIRST_VEHICLE + i;
    if (!is_exited)
    {
      p_n[v.head[0]][v.head[1]] = letter;
      for (int j = 0; j < v.length; j++)
      {
        int offset_i = v.orient == vertical ? j : 0;
        int offset_j = v.orient == horizontal ? j : 0;
        p_n[v.head[0] + offset_i][v.head[1] + offset_j] = letter;
      }
    }
  }
  for (int i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++)
    {
      printf("%c ", p_n[i][j]);
    }
    printf("\n");
  }
  printf("------------\n");
}

void print_path(initial_park ip, park pks[POSSIBILITIES])
{
  // must include itself
  static int indexes[POSSIBILITIES];
  indexes[0] = ip.best_park;
  int current_idx = ip.best_park;

  // retrieve parents chain
  for (int i = 0; i < pks[ip.best_park].steps; i++)
  {
    indexes[i + 1] = pks[current_idx].origin;
    current_idx = pks[current_idx].origin;
  }

  // generate parks
  char p_n[PARK_MAX_SIDE][PARK_MAX_SIDE];
  for (int s = pks[ip.best_park].steps; s >= 0; s--)
  {
    // create the skeleton of the park
    memcpy(p_n, ip.p, sizeof(ip.p));

    // place the vehicles
    for (int i = 0; i < pks[indexes[s]].vehicles_num; i++)
    {
      vehicle v = pks[indexes[s]].vehicles[i];
      bool is_exited = v.is_exited;
      char letter = FIRST_VEHICLE + i;
      !is_exited && (generate_vehicle(p_n, v, letter));
    }
    for (int i = 0; i < ip.h; i++)
    {
      for (int j = 0; j < ip.w; j++)
      {
        printf("%c ", p_n[i][j]);
      }
      printf("\n");
    }
    printf("\n");
  }
}

bool generate_vehicle(char p[PARK_MAX_SIDE][PARK_MAX_SIDE], vehicle v, char l)
{
  if (!p || p == NULL)
  {
    return false;
  }
  p[v.head[0]][v.head[1]] = l;
  for (int j = 0; j < v.length; j++)
  {
    int offset_i = v.orient == vertical ? j : 0;
    int offset_j = v.orient == horizontal ? j : 0;
    p[v.head[0] + offset_i][v.head[1] + offset_j] = l;
  }
  return true;
}

void get_options(initial_park *ip, park pks[POSSIBILITIES], int v_i)
{
  if (ip && pks)
  {
    vehicle v = pks[ip->cur_idx].vehicles[v_i];
    park new_p;

    int old_i = v.head[0];
    int old_j = v.head[1];

    // move one step to the left and right / up and down
    for (int i = 0; i < MOVE_DIRECTIONS; i++)
    {
      new_p = pks[ip->cur_idx];
      int new_i = i == 0 ? old_i + 1 : old_i - 1;
      int new_j = i == 0 ? old_j + 1 : old_j - 1;
      v.head[0] = v.orient == vertical ? new_i : old_i;
      v.head[1] = v.orient == horizontal ? new_j : old_j;
      move_by_orient(ip, pks, &new_p, v, v_i);
    }
  }
}

bool move_by_orient(initial_park *ip, park pks[POSSIBILITIES], park *np, vehicle v, int v_i)
{
  if (!ip)
  {
    return false;
  }
  if (is_valid_move(*ip, *np, v, v_i))
  {
    np->vehicles[v_i].head[0] = v.head[0];
    np->vehicles[v_i].head[1] = v.head[1];
    // TODO: handle bool return
    handle_at_exit(*ip, np, v, v_i);
    if (is_unique_park(pks, ip->possibilities, *np))
    {
      np->origin = ip->cur_idx;
      np->steps += 1;
      pks[ip->possibilities] = *np;
      ip->possibilities += 1;
    }
  }
  return true;
}

bool handle_at_exit(initial_park ip, park *pk, vehicle v, int v_i)
{
  if (!pk)
  {
    return false;
  }

  int axis = v.orient == horizontal ? 1 : 0;
  int axis_lng = v.orient == horizontal ? ip.w : ip.h;
  int head = v.head[axis];
  int tail = v.head[axis] + v.length - 1;
  int board_start = 0;
  int board_end = axis_lng - BORDER_WIDTH;

  if (head == board_start || tail == board_end)
  {
    pk->vehicles[v_i].is_exited = true;
    return true;
  }
  return false;
}

bool is_valid_move(initial_park ip, park pk, vehicle v, int v_i)
{
  // 1. check if is on the border && is not an exit
  int axis = v.orient == horizontal ? 1 : 0;
  int axis_lng = v.orient == horizontal ? ip.w : ip.h;
  char cur_cell = ip.p[v.head[0]][v.head[1]];
  int head = v.head[axis];
  int tail = v.head[axis] + v.length - 1;
  int board_start = 0;
  int board_end = axis_lng - BORDER_WIDTH;
  char tail_cell = v.orient == horizontal ? ip.p[v.head[0]][tail] : ip.p[tail][v.head[1]];

  if (head == board_start && cur_cell == BOLLARD)
  {
    return false;
  }
  if ((tail) == (board_end) && tail_cell == BOLLARD)
  {
    return false;
  }

  // 2. check if overlap another vehicle
  for (int i = 0; i < pk.vehicles_num; i++)
  {
    vehicle cur = pk.vehicles[i];

    // exclude itself
    if (i != v_i && !cur.is_exited)
    {
      // horizontal + horizontal (head + tail)
      if (v.orient == horizontal && cur.orient == horizontal)
      {
        // offset current index
        if ((cur.head[0] == v.head[0]) && cur.head[1] + cur.length - 1 == v.head[1])
        {
          return false;
        }
        if ((cur.head[0] == v.head[0]) && cur.head[1] == v.head[1] + v.length - 1)
        {
          return false;
        }
      }

      // horizontal + vertical (head + tail)
      if (v.orient == horizontal && cur.orient == vertical)
      {
        // offset current index
        if ((cur.head[1] == v.head[1]) && (v.head[0] >= cur.head[0] && v.head[0] <= cur.head[0] + cur.length - 1))
        {
          return false;
        }
        if ((cur.head[1] == v.head[1] + v.length - 1) && (v.head[0] >= cur.head[0] && v.head[0] <= cur.head[0] + cur.length - 1))
        {
          return false;
        }
      }

      // vertical + vertical(head + tail)
      if (v.orient == vertical && cur.orient == vertical)
      {
        // offset current index
        if ((cur.head[1] == v.head[1]) && cur.head[0] + cur.length - 1 == v.head[0])
        {
          return false;
        }
        if ((cur.head[1] == v.head[1]) && cur.head[0] == v.head[0] + v.length - 1)
        {
          return false;
        }
      }

      // vertical + horizontal(head + tail)
      if (v.orient == vertical && cur.orient == horizontal)
      {
        // offset current index
        if ((cur.head[0] == v.head[0]) && (v.head[1] >= cur.head[1] && v.head[1] <= cur.head[1] + cur.length - 1))
        {
          return false;
        }
        if ((cur.head[0] == v.head[0] + v.length - 1) && (v.head[1] >= cur.head[1] && v.head[1] <= cur.head[1] + cur.length - 1))
        {
          return false;
        }
      }
    }
  }

  return true;
}

bool is_finished_park(park p)
{
  for (int i = 0; i < p.vehicles_num; i++)
  {
    if (!p.vehicles[i].is_exited)
    {
      return false;
    }
  }
  return true;
}

bool validate_park(initial_park ip)
{
  // size is larger than min length
  if (ip.w < PARK_SHORT_SIDE || ip.h < PARK_SHORT_SIDE)
  {
    return false;
  }
  // one side >= 3, another >= 4
  if ((ip.w < PARK_LONG_SIDE) && (ip.h < PARK_LONG_SIDE))
  {
    return false;
  }

  // TODO: not sure if should refactor
  for (int i = 0; i < ip.h; i++)
  {
    for (int j = 0; j < ip.w; j++)
    {
      char current_cell = ip.p[i][j];
      // validate each cell
      bool is_border = i == 0 || i == ip.h - 1 || j == 0 || j == ip.w - 1;
      if (is_border && !is_valid_border_cell(current_cell))
      {
        return false;
      }
      if (!is_border && !is_valid_inner_cell(current_cell))
      {
        return false;
      }
    }
  }
  return true;
}

bool is_valid_border_cell(char c)
{
  switch (c)
  {
  case GAP:
  case BOLLARD:
    return true;
  default:
    return false;
  }
}

bool is_valid_inner_cell(char c)
{
  return c == GAP || isupper(c);
}

void test(void)
{
  park pk;
  park pk1;
  park pk_finished = {0};
  pk_finished.vehicles_num = 1;
  pk_finished.vehicles[0].head[0] = 1;
  pk_finished.vehicles[0].head[1] = 2;
  pk_finished.vehicles[0].orient = horizontal;
  pk_finished.vehicles[0].is_exited = true;
  pk_finished.vehicles[0].is_init = true;

  char invalid_char = 'a';

  char validate_no_v[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                      {BOLLARD, GAP, GAP, BOLLARD},
                                                      {BOLLARD, GAP, GAP, GAP},
                                                      {BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char validate_with_v_h[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                          {BOLLARD, 'A', 'A', BOLLARD},
                                                          {BOLLARD, GAP, GAP, GAP},
                                                          {BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char validate_with_v_v[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                          {BOLLARD, 'A', GAP, BOLLARD},
                                                          {BOLLARD, 'A', GAP, GAP},
                                                          {BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char validate_1v[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                    {BOLLARD, 'A', GAP, BOLLARD},
                                                    {BOLLARD, 'A', GAP, GAP},
                                                    {BOLLARD, GAP, BOLLARD, BOLLARD}};
  char valid_2v[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                 {BOLLARD, 'A', GAP, GAP, GAP, BOLLARD},
                                                 {BOLLARD, 'A', GAP, 'B', 'B', GAP},
                                                 {BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char valid_2v_1[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, 'A', GAP, 'B', 'B', GAP},
                                                   {BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char valid_2v_2[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, GAP, GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, 'A', 'A', 'B', 'B', GAP},
                                                   {BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char valid_2v_3[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'A', 'B', 'B', GAP, GAP},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char valid_2v_4[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, GAP},
                                                   {BOLLARD, 'B', 'B', 'B', GAP, GAP, BOLLARD},
                                                   {BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char valid_2v_5[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'B', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'B', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char valid_2v_6[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {GAP, 'B', 'B', 'B', GAP, BOLLARD},
                                                   {BOLLARD, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char valid_3v_1[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'A', 'C', 'C', GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, GAP},
                                                   {BOLLARD, 'B', 'B', 'B', GAP, GAP, BOLLARD},
                                                   {BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char valid_3v_2[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, 'A', 'C', 'C', GAP, GAP},
                                                   {BOLLARD, GAP, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, 'B', 'B', 'B', GAP, GAP, GAP},
                                                   {BOLLARD, BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char valid_4v_1[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, 'B', 'B', 'B', 'C', GAP, GAP},
                                                   {BOLLARD, 'A', GAP, GAP, 'C', GAP, BOLLARD},
                                                   {BOLLARD, 'A', GAP, GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, 'A', GAP, GAP, GAP, GAP, BOLLARD},
                                                   {GAP, 'A', 'D', 'D', 'D', 'D', BOLLARD},
                                                   {BOLLARD, BOLLARD, BOLLARD, BOLLARD, GAP, BOLLARD, BOLLARD}};
  char valid_5v_1[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, 'D', 'B', 'B', 'B', 'E', GAP, GAP},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, BOLLARD},
                                                   {GAP, 'D', 'C', 'C', 'C', 'E', GAP, BOLLARD},
                                                   {BOLLARD, 'D', GAP, GAP, GAP, 'E', GAP, BOLLARD},
                                                   {BOLLARD, GAP, GAP, GAP, GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, GAP, BOLLARD, GAP, BOLLARD, GAP, BOLLARD, BOLLARD}};
  char valid_8v_1[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, 'D', 'B', 'B', 'B', 'E', GAP},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', BOLLARD},
                                                   {GAP, 'D', 'C', 'C', 'C', 'E', BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'G', GAP, 'E', BOLLARD},
                                                   {BOLLARD, 'F', 'F', 'G', 'H', 'H', GAP},
                                                   {BOLLARD, GAP, BOLLARD, GAP, BOLLARD, GAP, BOLLARD}};
  char valid_8v_2[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, 'D', 'B', 'B', 'B', 'E', GAP},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'A', GAP, 'E', BOLLARD},
                                                   {GAP, 'D', 'C', 'C', 'C', 'E', BOLLARD},
                                                   {BOLLARD, 'D', GAP, 'G', GAP, 'E', BOLLARD},
                                                   {BOLLARD, 'F', 'F', 'G', 'H', 'H', GAP},
                                                   {BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD, GAP, BOLLARD}};
  char valid_10v_1[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD, GAP, BOLLARD},
                                                    {BOLLARD, 'D', 'B', 'B', 'B', 'E', GAP, GAP, GAP},
                                                    {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {GAP, 'D', 'C', 'C', 'C', 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'D', GAP, 'G', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'F', 'F', 'G', 'H', 'H', GAP, GAP, GAP},
                                                    {BOLLARD, GAP, 'I', 'I', 'I', 'I', 'I', 'I', GAP},
                                                    {BOLLARD, GAP, BOLLARD, GAP, BOLLARD, GAP, GAP, BOLLARD, BOLLARD}};
  char valid_10v_2[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, GAP, BOLLARD, BOLLARD, BOLLARD, GAP, BOLLARD},
                                                    {BOLLARD, 'D', 'B', 'B', 'B', 'E', GAP, GAP, GAP},
                                                    {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'D', GAP, 'A', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'D', 'C', 'C', 'C', 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'D', GAP, 'G', GAP, 'E', GAP, GAP, BOLLARD},
                                                    {BOLLARD, 'F', 'F', 'G', 'H', 'H', GAP, GAP, GAP},
                                                    {BOLLARD, GAP, 'I', 'I', 'I', 'I', 'I', 'I', GAP},
                                                    {BOLLARD, GAP, BOLLARD, GAP, BOLLARD, GAP, GAP, BOLLARD, BOLLARD}};
  char invalid_2v[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, 'A', GAP, GAP, GAP, BOLLARD},
                                                   {BOLLARD, 'A', GAP, 'C', 'C', GAP},
                                                   {BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char invalid_3v[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                   {BOLLARD, 'A', GAP, 'C', GAP, BOLLARD},
                                                   {BOLLARD, 'A', GAP, 'B', 'B', GAP},
                                                   {BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char no_exit_p[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                  {BOLLARD, GAP, GAP, BOLLARD},
                                                  {BOLLARD, GAP, GAP, BOLLARD},
                                                  {BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char null_p2[PARK_MAX_SIDE][PARK_MAX_SIDE];
  char invalidate_size_p[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD}};
  char invalidate_size_1[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD},
                                                          {BOLLARD, GAP, GAP},
                                                          {BOLLARD, BOLLARD, BOLLARD}};
  char invalid_letter_p[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                         {BOLLARD, invalid_char, GAP, BOLLARD},
                                                         {BOLLARD, GAP, GAP, GAP},
                                                         {BOLLARD, BOLLARD, BOLLARD, BOLLARD}};
  char bollard_middle_p[PARK_MAX_SIDE][PARK_MAX_SIDE] = {{BOLLARD, BOLLARD, BOLLARD, BOLLARD},
                                                         {BOLLARD, BOLLARD, GAP, BOLLARD},
                                                         {BOLLARD, GAP, GAP, GAP},
                                                         {BOLLARD, BOLLARD, BOLLARD, BOLLARD}};

  assert(is_valid_inner_cell(GAP) == true);
  assert(is_valid_inner_cell('A') == true);
  assert(is_valid_inner_cell(BOLLARD) == false);
  assert(is_valid_inner_cell('9') == false);

  assert(is_valid_border_cell(GAP) == true);
  assert(is_valid_border_cell(BOLLARD) == true);
  assert(is_valid_border_cell('A') == false);
  assert(is_valid_border_cell('9') == false);

  assert(string_to_brd(validate_no_v, NULL, 4, 4) == false);
  initial_park ip1 = {.possibilities = 1};
  assert(string_to_brd(validate_no_v, &ip1, 4, 4) == true);
  assert(validate_park(ip1) == true);
  initial_park ip2 = {.possibilities = 1};
  assert(string_to_brd(no_exit_p, &ip2, 4, 4) == true);
  assert(validate_park(ip2) == true);
  initial_park ip3 = {.possibilities = 1};
  assert(string_to_brd(null_p2, &ip3, 4, 4) == true);
  assert(validate_park(ip3) == false);
  initial_park ip4 = {.possibilities = 1};
  assert(string_to_brd(invalidate_size_p, &ip4, 0, 0) == true);
  assert(validate_park(ip4) == false);
  initial_park ip5 = {.possibilities = 1};
  assert(string_to_brd(invalidate_size_1, &ip5, 3, 3) == true);
  assert(validate_park(ip5) == false);
  initial_park ip6 = {.possibilities = 1};
  assert(string_to_brd(invalid_letter_p, &ip6, 4, 4) == true);
  assert(validate_park(ip6) == false);
  initial_park ip7 = {.possibilities = 1};
  assert(string_to_brd(bollard_middle_p, &ip7, 4, 4) == true);
  assert(validate_park(ip7) == false);

  // NULL pointer
  assert(get_vehicle_lng(NULL, 1, 1, 1) == 0);
  // Horizontal vehicle
  assert(get_vehicle_lng(validate_with_v_h, 1, 1, 1) == 2);
  assert(validate_with_v_h[1][1] == GAP);
  assert(validate_with_v_h[1][2] == GAP);
  // Vertical vehicle
  assert(get_vehicle_lng(validate_with_v_v, 1, 1, 0) == 2);
  assert(validate_with_v_v[1][1] == GAP);
  assert(validate_with_v_v[2][1] == GAP);

  // Insert vehicle A
  initial_park ip25 = {.possibilities = 1};
  assert(string_to_brd(valid_2v, &ip25, 4, 6) == true);
  assert(get_vehicle_info(&ip25, &pk, 1, 1) == true);
  assert(pk.vehicles[0].is_init == true);
  assert(pk.vehicles[0].length == 2);
  assert(pk.vehicles[0].orient == vertical);
  assert(pk.vehicles[0].is_exited == false);
  assert(pk.vehicles[0].head[0] == 1);
  assert(pk.vehicles[0].head[1] == 1);

  // Insert vehicle B
  assert(get_vehicle_info(&ip25, &pk, 2, 3) == true);
  assert(pk.vehicles[1].is_init == true);
  assert(pk.vehicles[1].length == 2);
  assert(pk.vehicles[1].orient == horizontal);
  assert(pk.vehicles[1].is_exited == false);
  assert(pk.vehicles[1].head[0] == 2);
  assert(pk.vehicles[1].head[1] == 3);

  // Insert vehicle C
  initial_park ip26 = {.possibilities = 1};
  assert(string_to_brd(invalid_3v, &ip26, 4, 6) == true);
  assert(get_vehicle_info(&ip26, &pk, 1, 3) == false);

  // 2 Vehicle in correct order
  initial_park ip8 = {.possibilities = 1};
  assert(string_to_brd(valid_2v_1, &ip8, 4, 6) == true);
  assert(get_vehicles_info(&ip8, &pk1) == true);
  assert(pk1.vehicles_num == 2);
  assert(generate_vehicle(NULL, pk1.vehicles[0], FIRST_VEHICLE) == false);

  vehicle v_b1 = {0};
  v_b1.head[0] = 2;
  v_b1.head[1] = 2;
  v_b1.orient = horizontal;
  v_b1.length = 2;
  v_b1.is_exited = false;
  v_b1.is_init = true;

  // Valid move: B to the left and get to an empty cell
  assert(is_valid_move(ip8, pk1, v_b1, 1) == true);

  // Valid move: B to the right and get to the exit
  v_b1.head[0] = 2;
  v_b1.head[1] = 4;
  assert(is_valid_move(ip8, pk1, v_b1, 1) == true);

  // Invalid move: B to the left and overlap A
  v_b1.head[0] = 2;
  v_b1.head[1] = 1;
  assert(is_valid_move(ip8, pk1, v_b1, 1) == false);

  // Invalid move: B to the right and get to the border
  v_b1.head[0] = 1;
  v_b1.head[1] = 4;
  assert(is_valid_move(ip8, pk1, v_b1, 1) == false);

  // Two new possibilities are added by moving B horizontally
  static park pks2[POSSIBILITIES];
  pks2[0] = pk1;
  get_options(&ip8, NULL, 1);
  get_options(NULL, pks2, 1);
  assert(ip8.possibilities == 1);
  get_options(&ip8, pks2, 1);
  assert(ip8.possibilities == 3);
  // One new possibility should be added by moving A vertically
  get_options(&ip8, pks2, 0);
  assert(ip8.possibilities == 4);
  assert(pks2[1].vehicles[1].head[1] == 4);
  assert(pks2[2].vehicles[1].head[1] == 2);
  assert(explore_parks(&ip8, pks2) == 2);

  // 2 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip9 = {.possibilities = 1};
  assert(string_to_brd(valid_2v_2, &ip9, 4, 6) == true);
  assert(get_vehicles_info(&ip9, &pk1) == true);
  assert(pk1.vehicles_num == 2);

  // Invalid move: A to the right and overlap B
  v_b1.head[0] = 2;
  v_b1.head[1] = 2;
  v_b1.orient = horizontal;
  v_b1.length = 2;
  v_b1.is_exited = false;
  assert(is_valid_move(ip9, pk1, v_b1, 0) == false);

  // Invalid move: A to the left and get to the border
  v_b1.head[0] = 2;
  v_b1.head[1] = 0;
  assert(is_valid_move(ip9, pk1, v_b1, 0) == false);

  // 2 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip10 = {.possibilities = 1};
  assert(string_to_brd(valid_2v_3, &ip10, 6, 7) == true);
  assert(get_vehicles_info(&ip10, &pk1) == true);
  assert(pk1.vehicles_num == 2);

  // Invalid move: B to the left and overlap A
  v_b1.head[0] = 3;
  v_b1.head[1] = 2;
  assert(is_valid_move(ip10, pk1, v_b1, 1) == false);

  // 2 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip11 = {.possibilities = 1};
  assert(string_to_brd(valid_2v_4, &ip11, 6, 7) == true);
  assert(get_vehicles_info(&ip11, &pk1) == true);
  assert(pk1.vehicles_num == 2);

  // Invalid move: A to the bottom and overlap B
  v_b1.head[0] = 2;
  v_b1.head[1] = 2;
  v_b1.orient = vertical;
  v_b1.length = 3;
  assert(is_valid_move(ip11, pk1, v_b1, 0) == false);

  // Invalid move: A to the top and and get to the border
  v_b1.head[0] = 0;
  v_b1.head[1] = 2;
  assert(is_valid_move(ip11, pk1, v_b1, 0) == false);

  // 3 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip12 = {.possibilities = 1};
  assert(string_to_brd(valid_3v_1, &ip12, 6, 7) == true);
  assert(get_vehicles_info(&ip12, &pk1) == true);
  assert(pk1.vehicles_num == 3);

  // Invalid move: C to the top and and get to the border
  v_b1.head[0] = 2;
  v_b1.head[1] = 2;
  v_b1.orient = horizontal;
  v_b1.length = 2;
  assert(is_valid_move(ip12, pk1, v_b1, 2) == false);

  // Valid move: C to the right and get to an empty cell
  v_b1.head[0] = 2;
  v_b1.head[1] = 4;
  assert(is_valid_move(ip12, pk1, v_b1, 2) == true);
  assert(handle_at_exit(ip12, &pk1, v_b1, 2) == false);
  assert(pk1.vehicles[2].is_exited == false);

  // 2 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip13 = {.possibilities = 1};
  assert(string_to_brd(valid_2v_5, &ip13, 6, 7) == true);
  assert(get_vehicles_info(&ip13, &pk1) == true);
  assert(pk1.vehicles_num == 2);

  // Valid move: B to the bottom and and get to the exit
  v_b1.head[0] = 4;
  v_b1.head[1] = 2;
  v_b1.orient = vertical;
  assert(is_valid_move(ip13, pk1, v_b1, 1) == true);
  assert(handle_at_exit(ip13, NULL, v_b1, 1) == false);
  assert(pk1.vehicles[1].is_exited == false);
  assert(pk1.vehicles[0].is_exited == false);
  assert(handle_at_exit(ip13, &pk1, v_b1, 1) == true);
  assert(pk1.vehicles[1].is_exited == true);
  assert(pk1.vehicles[0].is_exited == false);

  // Invalid move: B to the top and overlap A
  v_b1.head[0] = 2;
  v_b1.head[1] = 2;
  assert(is_valid_move(ip13, pk1, v_b1, 1) == false);

  // 2 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip14 = {.possibilities = 1};
  assert(string_to_brd(valid_2v_6, &ip14, 6, 6) == true);
  assert(get_vehicles_info(&ip14, &pk1) == true);
  assert(pk1.vehicles_num == 2);
  pks2[0] = pk1;
  assert(explore_parks(&ip14, pks2) == 3);

  // 4 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip15 = {.possibilities = 1};
  assert(string_to_brd(valid_4v_1, &ip15, 7, 7) == true);
  assert(get_vehicles_info(&ip15, &pk1) == true);
  assert(pk1.vehicles_num == 4);
  pks2[0] = pk1;
  assert(explore_parks(&ip15, pks2) == 11);

  // 3 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip16 = {.possibilities = 1};
  assert(string_to_brd(valid_3v_2, &ip16, 6, 7) == true);
  assert(get_vehicles_info(&ip16, &pk1) == true);
  assert(pk1.vehicles_num == 3);

  pks2[0] = pk1;
  // No possibilities should be added by moving A vertically
  get_options(&ip16, pks2, 0);
  assert(ip16.possibilities == 1);
  // One new possibility are added by moving B horizontally
  get_options(&ip16, pks2, 1);
  assert(ip16.possibilities == 2);
  assert(pks2[ip16.possibilities - 1].vehicles[1].head[0] == 4);
  assert(pks2[ip16.possibilities - 1].vehicles[1].head[1] == 2);
  assert(pks2[ip16.possibilities - 1].steps == 1);
  assert(pks2[ip16.possibilities - 1].origin == 0);
  // One new possibility are added by moving C horizontally
  get_options(&ip16, pks2, 2);
  assert(ip16.possibilities == 3);
  assert(pks2[ip16.possibilities - 1].vehicles[2].head[0] == 2);
  assert(pks2[ip16.possibilities - 1].vehicles[2].head[1] == 4);
  assert(pks2[ip16.possibilities - 1].steps == 1);
  assert(pks2[ip16.possibilities - 1].origin == 0);
  // One new possibility are added by moving C horizontally from previous move
  // Successfully exclude duplicate path
  // C reaches an exit and is successfully marked as is_exited
  ip16.cur_idx = ip16.possibilities - 1;
  get_options(&ip16, pks2, 2);
  assert(ip16.possibilities == 4);
  assert(pks2[ip16.possibilities - 1].vehicles[2].head[0] == 2);
  assert(pks2[ip16.possibilities - 1].vehicles[2].head[1] == 5);
  assert(pks2[ip16.possibilities - 1].vehicles[2].is_exited == true);
  assert(pks2[ip16.possibilities - 1].steps == 2);
  assert(pks2[ip16.possibilities - 1].origin == 2);
  assert(explore_parks(&ip16, pks2) == 7);
  // Unfinished park
  assert(is_finished_park(pk1) == false);

  // 1 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip17 = {.possibilities = 1};
  assert(string_to_brd(validate_1v, &ip17, 4, 4) == true);
  assert(get_vehicles_info(&ip17, &pk1) == true);
  assert(pk1.vehicles_num == 1);

  // A could successfully leave the park
  pks2[0] = pk1;
  assert(explore_parks(&ip17, pks2) == 1);

  // 5 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip18 = {.possibilities = 1};
  assert(string_to_brd(valid_5v_1, &ip18, 10, 8) == true);
  assert(get_vehicles_info(&ip18, &pk1) == true);
  assert(pk1.vehicles_num == 5);

  // 5 vehicles could successfully leave the park
  pks2[0] = pk1;
  assert(explore_parks(&ip18, pks2) == 13);

  // 8 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip19 = {.possibilities = 1};
  assert(string_to_brd(valid_8v_1, &ip19, 10, 7) == true);
  assert(get_vehicles_info(&ip19, &pk1) == true);
  assert(pk1.vehicles_num == 8);

  // 8 vehicles could successfully leave the park
  pks2[0] = pk1;
  assert(explore_parks(&ip19, pks2) == 16);

  // valid_8v_2
  // 8 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip20 = {.possibilities = 1};
  assert(string_to_brd(valid_8v_2, &ip20, 10, 7) == true);
  assert(get_vehicles_info(&ip20, &pk1) == true);
  assert(pk1.vehicles_num == 8);

  // 8 vehicles can not successfully leave the park
  pks2[0] = pk1;
  assert(explore_parks(&ip20, pks2) == 0);

  // 9 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip21 = {.possibilities = 1};
  assert(string_to_brd(valid_10v_1, &ip21, 11, 9) == true);
  assert(get_vehicles_info(&ip21, &pk1) == true);
  assert(pk1.vehicles_num == 9);

  // 9 vehicles could successfully leave the park
  pks2[0] = pk1;
  // assert(explore_parks(&ip21, pks2) == 26);

  // 9 Vehicle in correct order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip22 = {.possibilities = 1};
  assert(string_to_brd(valid_10v_2, &ip22, 11, 9) == true);
  assert(get_vehicles_info(&ip22, &pk1) == true);
  assert(pk1.vehicles_num == 9);

  // 9 vehicles can not successfully leave the park
  pks2[0] = pk1;
  // assert(explore_parks(&ip22, pks2) == 0);

  // 2 Vehicle in incorrect order
  memset(&pk1, 0, sizeof(pk1));
  initial_park ip23 = {.possibilities = 1};
  assert(string_to_brd(invalid_2v, &ip23, 4, 6) == true);
  assert(get_vehicles_info(&ip23, &pk1) == false);
  assert(pk1.vehicles_num == 2);

  // No vehicle in a park
  initial_park ip24 = {.possibilities = 1};
  assert(string_to_brd(validate_no_v, &ip24, 4, 4) == true);
  assert(get_vehicles_info(&ip24, &pk1) == false);

  // Finished park
  assert(is_finished_park(pk_finished) == true);

  // Duplicate park created
  park pk_dup1 = {0};
  pk_dup1.vehicles_num = 1;
  pk_dup1.vehicles[0].head[0] = 1;
  pk_dup1.vehicles[0].head[1] = 2;
  pk_dup1.vehicles[0].orient = horizontal;
  pk_dup1.vehicles[0].is_init = true;

  park pks1[3];
  pks1[0] = pk_dup1;
  park pk_dup2 = pk_dup1;

  assert(is_unique_park(pks1, 1, pk_dup2) == false);

  // Unique park created: Different i in Head
  park pk_unique1 = pk_dup1;
  pk_unique1.vehicles[0].head[0] = 2;

  // Unique park created: Different j in Head
  park pk_unique2 = pk_dup1;
  pk_unique2.vehicles[0].head[1] = 3;

  // Unique park created: Different is_exit status
  park pk_unique3 = pk_dup1;
  pk_unique3.vehicles[0].is_exited = true;

  assert(is_unique_park(pks1, 1, pk_unique1) == true);
  assert(is_unique_park(pks1, 1, pk_unique2) == true);
  assert(is_unique_park(pks1, 1, pk_unique3) == true);
}
