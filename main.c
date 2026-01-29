#if defined(WIN32)
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif
#include <stdio.h>

#define TAB_WIDTH 4

#define MAX_LINE_LENGTH 1000
#define MAX_LINE_COUNT 10000

#define MAX_FILENAME_LENGTH 1000

typedef struct {
	char data[MAX_LINE_LENGTH];
	int length;
} line_t;

typedef struct {
	line_t lines[MAX_LINE_COUNT];
	int line_count;
	
	int cursor_x;
	int cursor_y;
	
	int view_data_offset_y;
	
	char filename[MAX_FILENAME_LENGTH];
	int filename_length;

	int needs_saving;
} editor_state_t;

// ====== Custom min and max to remove dependencies ======

int fox_min(int a, int b){
	return (a < b) ? a : b;
}

int fox_max(int a, int b){
	return (a > b) ? a : b;
}

// ====== Debug Print Functions ======

void debug_print_line(const line_t* line, int y, int line_index, int max_width){
	move(y, 0);
	clrtoeol();
	mvwprintw(stdscr, y, 0, "%5d ", line_index + 1);
	for (int i = 0; i < line->length && i < max_width - 6; i++){
		if (line->data[i] == '\t'){
			for (int t = 0; t < TAB_WIDTH; t++){
				addch(' ');
			}
		}else{
			addch(line->data[i]);
		}
	}
}

void debug_print_in_view(editor_state_t* state, int view_height, int view_offset_y){
	int amount_y = fox_min(view_height, state->line_count);
	int data_offset_y = state->view_data_offset_y;
	data_offset_y = fox_min(data_offset_y, state->cursor_y);
	data_offset_y = fox_max(data_offset_y, state->cursor_y - amount_y + 1);
	data_offset_y = fox_max(0, data_offset_y);
	amount_y = fox_min(amount_y, state->line_count - data_offset_y);
	state->view_data_offset_y = data_offset_y;
	for (int i = 0; i < amount_y; i++){
		debug_print_line(&state->lines[i + data_offset_y], i + view_offset_y, i + data_offset_y, COLS);
	}
	for (int i = amount_y; i < view_height; i++){
		move(i, 0);
		clrtoeol();
	}
}

void set_cursor_in_view(editor_state_t* state, int view_offset_y){
	int y = (state->cursor_y - state->view_data_offset_y) + view_offset_y;
	int x = state->cursor_x + 6;
	for(int i = 0; i < fox_min(state->lines[state->cursor_y].length, state->cursor_x); i++){
		if (state->lines[state->cursor_y].data[i] == '\t'){
			x += TAB_WIDTH - 1;
		}
	}
	x = fox_min(x, COLS - 1);
	move(y, x);
}

void render_all(editor_state_t* state){
	debug_print_in_view(state, LINES-1, 0);
	move(LINES-1, 0);
	attron(A_STANDOUT);
	for (int i = 0; i < COLS; i++){
		addch(' ');
	}
	if (state->needs_saving){
		mvwprintw(stdscr, LINES-1, COLS-state->filename_length, "*");
	}
	mvwprintw(stdscr, LINES-1, COLS-state->filename_length + 1, "%s", state->filename);
	attroff(A_STANDOUT);
	set_cursor_in_view(state, 0);
}

// ====== Cursor Management Functions ======

void limit_cursor_to_bounds(editor_state_t* state){
	state->cursor_y = fox_max(state->cursor_y, 0);
	state->cursor_x = fox_max(state->cursor_x, 0);
	
	state->cursor_y = fox_min(state->cursor_y, state->line_count - 1);
	state->cursor_x = fox_min(state->cursor_x, state->lines[state->cursor_y].length);
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

// ====== Character Management Functions ======

void remove_characters(editor_state_t* state, int line_index, int start, int len){
	if (state->cursor_x == 0) {
		if (state->lines[state->cursor_y].length == 0){
			remove_lines(state, state->cursor_y, 1);
		}else{
			state->cursor_y -= 1;
			limit_cursor_to_bounds(state);
			state->cursor_x = state->lines[state->cursor_y].length;
		}
		return;
	}
	for (int i = start; i < state->lines[line_index].length - len; i++){
		state->lines[line_index].data[i] = state->lines[line_index].data[i+len];
	}
	state->lines[line_index].length -= len;
	state->cursor_x -= 1;
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

// ====== File Operation Functions ======
void read_file(editor_state_t* state){
	const char* filename = state->filename;
	FILE* file = fopen(filename, "r");
	if (!file){
		state->needs_saving = 1;
		return;
	}
	state->line_count = 1;
	state->lines[0].length = 0;
	state->cursor_x = 0;
	state->cursor_y = 0;
	char c[1];
	int read_length;
	read_length = fread(c, sizeof(c), 1, file);
	while (read_length == 1){
		if (c[0] == '\n'){
			create_newline(state, state->cursor_y);
		} else {
			int line_index = state->cursor_y;
			int pos = state->cursor_x;
			insert_character(state, line_index, pos, c[0]);
		}
		read_length = fread(c, sizeof(c), 1, file);
	}
	fclose(file);
}

void write_file(editor_state_t* state){
	if (state->filename_length == 0){
		return;
	}
	const char* filename = state->filename;
	FILE* file = fopen(filename, "w");
	if (!file){
		return;
	}
	char c[1];
	for (int line = 0; line < state->line_count; line++){
		for (int i = 0; i < state->lines[line].length; i++) {
			c[0] = state->lines[line].data[i];
			fwrite(c, sizeof(c), 1, file);
		}
		if (line != state->line_count - 1) {
			c[0] = '\n';
			fwrite(c, sizeof(c), 1, file);
		}
	}
	fclose(file);
	state->needs_saving = 0;
}

// ====== Key Handling ======

void send_key(editor_state_t* state, int key){
	switch(key){
		case(10):
		case(KEY_ENTER):
			create_newline(state, state->cursor_y);
			state->needs_saving = 1;
			break;
		case(KEY_BACKSPACE): {
			int line_index = state->cursor_y;
			int start = state->cursor_x - 1;
			int len = 1;
			remove_characters(state, line_index, start, len);
			state->needs_saving = 1;
			break;
		}
		case(KEY_DC): {
			int line_index = state->cursor_y;
			int start = state->cursor_x;
			int len = 1;
			if (start != state->lines[line_index].length){
				remove_characters(state, line_index, start, len);
				state->cursor_x = start;
				state->needs_saving = 1;
			}
			break;
		}
		case(KEY_LEFT): {
			state->cursor_x -= 1;
			break;
		}
		case(KEY_RIGHT): {
			state->cursor_x += 1;
			break;
		}
		case(KEY_UP): {
			state->cursor_y -= 1;
			break;
		}
		case(KEY_DOWN): {
			state->cursor_y += 1;
			break;
		}
		case(KEY_HOME): {
			state->cursor_x = 0;
			break;
		}
		case(KEY_END): {
			state->cursor_x = state->lines[state->cursor_y].length;
			break;
		}
		case(KEY_PPAGE):  { // Page up
			state->view_data_offset_y -= LINES - 4;
			state->cursor_y = state->view_data_offset_y + LINES - 1;
			break;
		}
		case(KEY_NPAGE):  { // Page down
			state->view_data_offset_y += LINES - 4;
			state->cursor_y = state->view_data_offset_y;
			break;
		}

#ifdef KEY_RESIZE
		case(KEY_RESIZE): { break; }
#endif

		case(19): {
			write_file(state);
			break;
		}
		case(27):{ break; }
		default: {
			int line_index = state->cursor_y;
			int pos = state->cursor_x;
			insert_character(state, line_index, pos, key);
			state->needs_saving = 1;
			break;
		}
	}
	limit_cursor_to_bounds(state);
}

int main(int argc, char* argv[]){
	initscr();
	noecho();
	cbreak();
	curs_set(1);
	keypad(stdscr, TRUE);
	
	static editor_state_t editor_state;
	editor_state.line_count = 1;
	editor_state.lines[0].length = 0;
	editor_state.cursor_x = 0;
	editor_state.cursor_y = 0;
	editor_state.filename_length = 0;
	editor_state.needs_saving = 0;
	editor_state.view_data_offset_y = 0;
	
	if (argc >= 2){
		while(
			editor_state.filename_length < MAX_FILENAME_LENGTH - 1\
			&& argv[1][editor_state.filename_length] != '\0'
		){
			editor_state.filename[editor_state.filename_length] = argv[1][editor_state.filename_length];
			editor_state.filename_length += 1;
		}
		editor_state.filename[editor_state.filename_length] = '\0';
		editor_state.filename_length += 1;
	}
	
	if (editor_state.filename_length > 0){
		read_file(&editor_state);
	}

	editor_state.cursor_x = 0;
	editor_state.cursor_y = 0;
	
	char last_key = ' ';
	render_all(&editor_state);
	while (1){
		int key = getch();
		send_key(&editor_state, key);
		render_all(&editor_state);

		//mvwprintw(stdscr, LINES-1, 0, "%d", key);
		//set_cursor_in_view(&editor_state, 0);
	}
	
	endwin();
	
	return 0;
}
