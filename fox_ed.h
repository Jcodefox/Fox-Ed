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

	int needs_saving;
} editor_state_t;

// ====== Functions Library User Must Implement ======
void set_cursor_on_screen(int x, int y);
void clear_line(int line_number);
void put_char_at(int x, int y, char c);
void write_file(editor_state_t* state); // TODO: IO Interface can't be required

// ====== Functions Library User Can Use ======
void limit_cursor_to_bounds(editor_state_t* state);

void set_cursor_in_view(editor_state_t* state, int view_offset_y);
void print_line_at(const line_t* line, int y, int line_index, int max_width);
void print_in_view(editor_state_t* state, int view_height, int view_offset_y);

void copy_line_to_line(editor_state_t* state, int index_src, int index_dest);
void create_newline(editor_state_t* state, int pos);
void remove_lines(editor_state_t* state, int start, int len);
void clear_all_lines(editor_state_t* state);

int remove_characters(editor_state_t* state, int line_index, int start, int len);
void backspace(editor_state_t* state, int line_index, int start, int len);
void insert_character(editor_state_t* state, int line_index, int pos, char character);

void send_special_key(editor_state_t* state, int key);
void type_character(editor_state_t *state, int ch);

//#define FOX_ED_IMPLEMENTATION
#ifdef FOX_ED_IMPLEMENTATION
// ====== Custom min and max to remove dependencies ======

int fox_min(int a, int b){
	return (a < b) ? a : b;
}

int fox_max(int a, int b){
	return (a > b) ? a : b;
}

// ====== Cursor Management Functions ======

void limit_cursor_to_bounds(editor_state_t* state){
	state->cursor_y = fox_max(state->cursor_y, 0);
	state->cursor_x = fox_max(state->cursor_x, 0);
	
	state->cursor_y = fox_min(state->cursor_y, state->line_count - 1);
	state->cursor_x = fox_min(state->cursor_x, state->lines[state->cursor_y].length);
}

// ======= Render Functions ======

void set_cursor_in_view(editor_state_t* state, int view_offset_y){
	int y = (state->cursor_y - state->view_data_offset_y) + view_offset_y;
	int x = state->cursor_x + 6;
	for(int i = 0; i < fox_min(state->lines[state->cursor_y].length, state->cursor_x); i++){
		if (state->lines[state->cursor_y].data[i] == '\t'){
			x += TAB_WIDTH - 1;
		}
	}
	x = fox_min(x, state->screen_width - 1);
	set_cursor_on_screen(x, y);
}

void print_line_at(const line_t* line, int y, int line_index, int max_width){
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
	for (int i = 0; i < line->length && i < max_width - 6; i++){
		if (line->data[i] == '\t'){
			for (int t = 0; t < TAB_WIDTH; t++){
				put_char_at(x++, y, ' ');
			}
		}else{
			put_char_at(x++, y, line->data[i]);
		}
	}
}

void print_in_view(editor_state_t* state, int view_height, int view_offset_y){
	int amount_y = fox_min(view_height, state->line_count);
	int data_offset_y = state->view_data_offset_y;
	data_offset_y = fox_min(data_offset_y, state->cursor_y);
	data_offset_y = fox_max(data_offset_y, state->cursor_y - amount_y + 1);
	data_offset_y = fox_max(0, data_offset_y);
	amount_y = fox_min(amount_y, state->line_count - data_offset_y);
	state->view_data_offset_y = data_offset_y;
	for (int i = 0; i < amount_y; i++){
		print_line_at(&state->lines[i + data_offset_y], i + view_offset_y, i + data_offset_y, state->screen_width);
	}
	for (int i = amount_y; i < view_height; i++){
		clear_line(i);
	}
}

// ====== Line Management Functions ======

void copy_line_to_line(editor_state_t* state, int index_src, int index_dest){
	state->lines[index_dest].length = state->lines[index_src].length;
	for (int i = 0; i < state->lines[index_src].length; i++){
		state->lines[index_dest].data[i] = state->lines[index_src].data[i];
	}
}

void create_newline(editor_state_t* state, int pos){
	const int len = 1;
	if (state->line_count + len > MAX_LINE_COUNT){
		return;
	}
	state->line_count += len;
	for (int i = state->line_count - 1; i >= pos + len; i--){
		copy_line_to_line(state, i-len, i);
	}
	state->cursor_y += len;
	state->cursor_x = 0;
	state->lines[state->cursor_y].length = 0;
}

void remove_lines(editor_state_t* state, int start, int len){
	if (state->cursor_y == 0) {
		return;
	}
	for (int i = start; i < state->line_count - len; i++){
		copy_line_to_line(state, i+len, i);
	}
	state->line_count -= len;
	state->cursor_y -= 1;
	state->cursor_x = state->lines[state->cursor_y].length;
}

void clear_all_lines(editor_state_t* state){
	state->line_count = 1;
	state->lines[0].length = 0;
	state->cursor_x = 0;
	state->cursor_y = 0;
	state->needs_saving = 1;
}

// ====== Character Management Functions ======

int remove_characters(editor_state_t* state, int line_index, int start, int len){
	for (int i = start; i < state->lines[line_index].length - len; i++){
		state->lines[line_index].data[i] = state->lines[line_index].data[i+len];
	}
	state->lines[line_index].length -= len;
	return 1;
}

void backspace(editor_state_t* state, int line_index, int start, int len){
	if (state->cursor_x == 0){
		if (state->lines[state->cursor_y].length == 0){
			remove_lines(state, state->cursor_y, 1);

		}else{
			state->cursor_y -= 1;
			limit_cursor_to_bounds(state);
			state->cursor_x = state->lines[state->cursor_y].length;
		}
		return;
	}

	if (remove_characters(state, line_index, start, len)){
		state->cursor_x -= 1;
	}
}

void insert_character(editor_state_t* state, int line_index, int pos, char character){
	const int len = 1;
	if (state->lines[line_index].length + len > MAX_LINE_LENGTH){
		return;
	}
	state->lines[line_index].length += len;
	for (int i = state->lines[line_index].length - 1; i >= pos + len; i--){
		state->lines[line_index].data[i] = state->lines[line_index].data[i-len];
	}
	state->lines[line_index].data[state->cursor_x] = character;
	state->cursor_x += len;
}

// ====== Key Handling ======
void send_special_key(editor_state_t* state, int key){
	int skip_set_furthest_right = 0;
	switch(key){
		case(FOX_KEY_ENTER): {
			create_newline(state, state->cursor_y);
			state->needs_saving = 1;
			break;
		}
		case(FOX_KEY_BACKSPACE): {
			int line_index = state->cursor_y;
			int start = state->cursor_x - 1;
			int len = 1;
			backspace(state, line_index, start, len);
			state->needs_saving = 1;
			break;
		}
		case(FOX_KEY_DELETE): {
			int line_index = state->cursor_y;
			int start = state->cursor_x;
			int len = 1;
			if (start != state->lines[line_index].length){
				remove_characters(state, line_index, start, len);
				state->needs_saving = 1;
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

void type_character(editor_state_t* state, int ch){
	int line_index = state->cursor_y;
	int pos = state->cursor_x;
	insert_character(state, line_index, pos, ch);
	state->needs_saving = 1;
	limit_cursor_to_bounds(state);
	state->furthest_right = state->cursor_x;
}
#endif

#endif
