#ifndef FOX_ED_H
#define FOX_ED_H

#ifndef TAB_WIDTH
#define TAB_WIDTH 4
#endif

#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 1000
#endif

#ifndef MAX_LINE_COUNT
#define MAX_LINE_COUNT 10000
#endif

#ifndef MAX_FILENAME_LENGTH
#define MAX_FILENAME_LENGTH 1000
#endif

#define FOX_KEY_ENTER 0
#define FOX_KEY_BACKSPACE 1
#define FOX_KEY_DELETE 2
#define FOX_KEY_LEFT 3
#define FOX_KEY_RIGHT 4
#define FOX_KEY_UP 5
#define FOX_KEY_DOWN 6
#define FOX_KEY_HOME 7
#define FOX_KEY_END 8
#define FOX_KEY_PAGE_UP 9
#define FOX_KEY_PAGE_DOWN 10
#define FOX_KEY_SAVE 11

typedef struct {
	char data[MAX_LINE_LENGTH];
	int length;
	int visual_length;
} line_t;

typedef struct {
	line_t lines[MAX_LINE_COUNT];
	int line_count;
	
	int cursor_x;
	int cursor_y;

	int furthest_right;
	
	int view_data_offset_y;
	int screen_width;
	int screen_height;
	
	char filename[MAX_FILENAME_LENGTH];
	int filename_length;

	int edited_since_saving;
} editor_state_t;

// ====== Functions Library User Must Implement ======
void set_cursor_on_screen(int x, int y);
void clear_line(int line_number);
void put_char_at(int x, int y, char c);
void write_file(editor_state_t* state); // TODO: IO Interface can't be required

// ====== Functions Library User Can Use ======
void limit_cursor_to_bounds(editor_state_t* state);

void print_in_view(editor_state_t* state, int view_height, int view_offset_y);

void copy_line_to_line(editor_state_t* state, int index_src, int index_dest);
void create_newline_at_cursor(editor_state_t* state);
void remove_lines(editor_state_t* state, int start, int len);
void clear_all_lines(editor_state_t* state);

int remove_characters(editor_state_t* state, int line_index, int start, int len);
void backspace_at_cursor(editor_state_t* state, int len);
void insert_character_at_cursor(editor_state_t* state, char character);

void send_special_key(editor_state_t* state, int key);

#define FOX_ED_IMPLEMENTATION
#ifdef FOX_ED_IMPLEMENTATION
// ====== Custom min and max to remove dependencies ======

int fox_min(int a, int b){
	return (a < b) ? a : b;
}

int fox_max(int a, int b){
	return (a > b) ? a : b;
}

int fox_clamp(int n, int a, int b){
	return fox_min(fox_max(n, a), b);
}

// ====== Cursor Management Functions ======

void limit_cursor_to_bounds(editor_state_t* state){
	state->cursor_y = fox_clamp(state->cursor_y, 0, state->line_count - 1);
	state->cursor_x = fox_clamp(state->cursor_x, 0, state->lines[state->cursor_y].length);
}

// ======= Render Functions ======

int get_line_visual_width(editor_state_t* state, int line){
	int len = state->lines[line].length;
	for (int i = 0; i < state->lines[line].length; i++){
		if (state->lines[line].data[i] == '\t'){
			len += TAB_WIDTH - 1;
		}
	}
	return len;
}

int get_line_visual_height(editor_state_t* state, int line){
	int len = fox_max(1, get_line_visual_width(state, line));
	return (len / (state->screen_width - 6));
}

int get_view_cursor_x(editor_state_t* state, int view_offset_y){
	int x = state->cursor_x;
	for(int i = 0; i < fox_min(state->lines[state->cursor_y].length, state->cursor_x); i++){
		if (state->lines[state->cursor_y].data[i] == '\t'){
			x += TAB_WIDTH - 1;
		}
	}
	x = x % (state->screen_width - 6);
	x += 6;
	x = fox_min(x, state->screen_width - 1);
	return x;
}

int get_view_cursor_y(editor_state_t *state, int view_offset_y){
	int y = (state->cursor_y - state->view_data_offset_y) + view_offset_y;
	for (int i = state->view_data_offset_y; i < state->cursor_y; i++){
		y += get_line_visual_height(state, i);
	}
	y += state->cursor_x / (state->screen_width - 6);
	return y;
}

void set_cursor_in_view(editor_state_t* state, int view_offset_y){
	int x = get_view_cursor_x(state, view_offset_y);
	int y = get_view_cursor_y(state, view_offset_y);
	set_cursor_on_screen(x, y);
}

int print_line_at(const line_t* line, int y, int line_index, int max_width){
	clear_line(y);
	int val_to_print = line_index + 1;
	int printed_above_zero = 0;
	int x = 0;
	for (int i = 0; i < 5; i++){
		int num = ((val_to_print / 10000) % 10);
		val_to_print *= 10;
		if (num != 0){
			printed_above_zero = 1;
		}
		if (printed_above_zero){
			put_char_at(x++, y, '0' + num);
		}else{
			put_char_at(x++, y, ' ');
		}
	}
	put_char_at(x++, y, ' ');
	int lines_printed = 1;
	for (int i = 0; i < line->length; i++){
		if (line->data[i] == '\t'){
			for (int t = 0; t < TAB_WIDTH; t++){
				put_char_at(x++, y, ' ');
			}
		}else{
			put_char_at(x++, y, line->data[i]);
		}
		
		if (x >= max_width && i != line->length){
			y += 1;
			clear_line(y);
			x = 6;
			lines_printed += 1;
		}
	}
	return lines_printed;
}

//TODO: Simplify lines below
int how_many_to_print(editor_state_t* state, int view_height, int data_offset){
	int amount_y = fox_min(view_height, state->line_count);
	int sum = 0;
	for (int i = 0; i < amount_y; i++){
		sum += get_line_visual_height(state, data_offset + i) + 1;
		if (sum >= view_height){
			return i + 1;
		}
	}
	return amount_y;
}

int get_cursor_y_visual_dist_to_top_of_view(editor_state_t *state){
	int y = state->cursor_y;//(state->cursor_y - state->view_data_offset_y);
	y += state->cursor_x / (state->screen_width - 6);
	for (int i = state->view_data_offset_y; i < state->cursor_y; i++){
		y += get_line_visual_height(state, i);
	}
	return y;
}

void print_in_view(editor_state_t* state, int view_height, int view_offset_y){
	view_height = 10;
	
	int c_y = state->cursor_y;
	int amount_y = how_many_to_print(state, view_height, state->view_data_offset_y);
	int down_distance_to_scroll = fox_max(0, c_y - amount_y + 1);
	int data_offset_y = fox_clamp(state->view_data_offset_y, down_distance_to_scroll, c_y);
	state->view_data_offset_y = data_offset_y;
	
	int overflow_offset = 0;
	for (int i = 0; i < amount_y; i++){
		line_t* line = &state->lines[i + data_offset_y];
		int visual_location = i + view_offset_y + overflow_offset;
		int line_number = i + data_offset_y;
		int width = state->screen_width;
		
		int lines_taken = print_line_at(line, visual_location, line_number, width);
		overflow_offset += lines_taken - 1;
	}
	for (int i = amount_y; i < view_height; i++){
		clear_line(i);
	}
}

// Below functions are likely simplified enough
// ====== Line Management Functions ======
void copy_line_to_line(editor_state_t* state, int index_src, int index_dest){
	state->lines[index_dest].length = state->lines[index_src].length;
	state->lines[index_dest].visual_length = state->lines[index_src].visual_length;
	for (int i = 0; i < state->lines[index_src].length; i++){
		state->lines[index_dest].data[i] = state->lines[index_src].data[i];
	}
}

void create_newline_at_cursor(editor_state_t* state){
	const int pos = state->cursor_y;
	const int len = 1;
	if (state->line_count + len > MAX_LINE_COUNT){
		return;
	}
	state->line_count += len;
	for (int i = state->line_count - 1; i >= pos + len; i--){
		int src = i-len;
		int dest = i;
		copy_line_to_line(state, src, dest);
	}
	state->cursor_y += len;
	state->cursor_x = 0;
	state->lines[state->cursor_y].length = 0;
	state->lines[state->cursor_y].visual_length = 0;
}

void remove_lines(editor_state_t* state, int start, int len){
	for (int i = start; i < state->line_count - len; i++){
		int src = i+len;
		int dest = i;
		copy_line_to_line(state, src, dest);
	}
	state->line_count -= len;
	state->line_count = fox_max(state->line_count, 0);
}

void clear_all_lines(editor_state_t* state){
	state->line_count = 1;
	state->lines[0].length = 0;
	state->lines[0].visual_length = 0;
	state->cursor_x = 0;
	state->cursor_y = 0;
	state->edited_since_saving = 1;
}

// ====== Character Management Functions ======
int remove_characters(editor_state_t* state, int line_index, int start, int len){
	for (int i = start; i < start + len; i++){
		if (state->lines[line_index].data[i] == '\t'){
			state->lines[line_index].visual_length -= TAB_WIDTH;
		}else{
			state->lines[line_index].visual_length -= 1;
		}
	}
	for (int i = start; i < state->lines[line_index].length - len; i++){
		state->lines[line_index].data[i] = state->lines[line_index].data[i+len];
	}
	state->lines[line_index].length -= len;
	state->lines[line_index].length = fox_max(state->lines[line_index].length, 0);
	return 1;
}

void backspace_at_cursor(editor_state_t* state, int len){
	int line_index = state->cursor_y;
	int start = state->cursor_x - 1;
	if (state->cursor_x - len < 0){
		if (state->lines[state->cursor_y].length - len < 0 && state->cursor_y != 0){
			remove_lines(state, state->cursor_y, 1);
		}
		state->cursor_y -= 1;
		limit_cursor_to_bounds(state);
		state->cursor_x = state->lines[state->cursor_y].length;
		return;
	}

	if (remove_characters(state, line_index, start, len)){
		state->cursor_x -= len;
	}
}

void insert_character_at_cursor(editor_state_t* state, char character){
	const int pos = state->cursor_x;
	const int line_index = state->cursor_y;
	const int len = 1;
	
	// Don't bother inserting character if it would overrun line
	if (state->lines[line_index].length + len > MAX_LINE_LENGTH){
		return;
	}
	
	state->lines[line_index].length += len;
	if (character == '\t') {
		state->lines[line_index].visual_length += len * TAB_WIDTH;
	} else {
		state->lines[line_index].visual_length += len;
	}
	// Shuffle characters after the cursor to the right to make room for the new character
	for (int i = state->lines[line_index].length - 1; i >= pos + len; i--){
		state->lines[line_index].data[i] = state->lines[line_index].data[i-len];
	}
	state->lines[line_index].data[pos] = character;
	
	state->cursor_x += len;
	
	state->edited_since_saving = 1;
	limit_cursor_to_bounds(state);
	state->furthest_right = state->cursor_x;
}

// ====== Key Handling ======
void send_special_key(editor_state_t* state, int key){
	int skip_set_furthest_right = 0;
	switch(key){
		case(FOX_KEY_ENTER): {
			create_newline_at_cursor(state);
			state->edited_since_saving = 1;
			break;
		}
		case(FOX_KEY_BACKSPACE): {
			const int len = 1;
			backspace_at_cursor(state, len);
			state->edited_since_saving = 1;
			break;
		}
		case(FOX_KEY_DELETE): {
			int line_index = state->cursor_y;
			int start = state->cursor_x;
			const int len = 1;
			if (start != state->lines[line_index].length){
				remove_characters(state, line_index, start, len);
				state->edited_since_saving = 1;
			}
			break;
		}
		case(FOX_KEY_LEFT): {
			state->cursor_x -= 1;
			if (state->cursor_x < 0 && state->cursor_y > 0){
				state->cursor_y -= 1;
				state->cursor_x = state->lines[state->cursor_y].length;
			}
			break;
		}
		case(FOX_KEY_RIGHT): {
			state->cursor_x += 1;
			if (state->cursor_x > state->lines[state->cursor_y].length){
				state->cursor_y += 1;
				state->cursor_x = 0;
			}
			break;
		}
		
		case(FOX_KEY_UP): {
			state->cursor_y -= 1;
			state->cursor_x = state->furthest_right;
			skip_set_furthest_right = 1;
			break;
		}
		case(FOX_KEY_DOWN): {
			state->cursor_y += 1;
			state->cursor_x = state->furthest_right;
			skip_set_furthest_right = 1;
			break;
		}
		case(FOX_KEY_HOME): {
			state->cursor_x = 0;
			state->furthest_right = 0;
			skip_set_furthest_right = 1;
			break;
		}
		case(FOX_KEY_END): {
			state->cursor_x = state->lines[state->cursor_y].length;
			state->furthest_right = MAX_LINE_LENGTH;
			skip_set_furthest_right = 1;
			break;
		}
		case(FOX_KEY_PAGE_UP):  { // Page up
			state->view_data_offset_y -= state->screen_height - 4;
			state->cursor_y = state->view_data_offset_y + state->screen_height - 1;
			state->cursor_x = state->furthest_right;
			skip_set_furthest_right = 1;
			break;
		}
		case(FOX_KEY_PAGE_DOWN):  { // Page down
			state->view_data_offset_y += state->screen_height - 4;
			state->cursor_y = state->view_data_offset_y;
			state->cursor_x = state->furthest_right;
			skip_set_furthest_right = 1;
			break;
		}
		case(FOX_KEY_SAVE): {
			write_file(state);
			skip_set_furthest_right = 1;
			break;
		}
		default: {
			skip_set_furthest_right = 1;
			break;
		}
	}
	limit_cursor_to_bounds(state);
	if (!skip_set_furthest_right){
		state->furthest_right = state->cursor_x;
	}
}
#endif

#endif
