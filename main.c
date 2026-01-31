#define FOX_ED_IMPLEMENTATION
#include "fox_ed.h"

#if defined(WIN32)
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif
#include <stdio.h>

void initialize_ncurses(){
	initscr();
	noecho();
	cbreak();
	curs_set(1);
	keypad(stdscr, TRUE);
}

// ====== Interface Functions for fox_ed to use ncurses ======
void set_cursor_on_screen(int x, int y){
	move(y, x);
}

void clear_line(int line_number){
	move(line_number, 0);
	clrtoeol();
}

void put_char_at(int x, int y, char c){
	move(y, x);
	addch(c);
}

// ====== Print Functions ======
void render_all(editor_state_t* state){
	print_in_view(state, state->screen_height-1, 0);
	move(state->screen_height-1, 0);
	attron(A_STANDOUT);
	for (int i = 0; i < state->screen_width; i++){
		addch(' ');
	}
	if (state->needs_saving){
		mvwprintw(stdscr, state->screen_height-1, state->screen_width-state->filename_length, "*");
	}
	mvwprintw(stdscr, state->screen_height-1, state->screen_width-state->filename_length + 1, "%s", state->filename);
	attroff(A_STANDOUT);
	set_cursor_in_view(state, 0);
}

// ====== File Operation Functions ======
void read_file(editor_state_t* state){
	const char* filename = state->filename;
	FILE* file = fopen(filename, "r");
	if (!file){
		state->needs_saving = 1;
		return;
	}
	clear_all_lines(state);
	state->needs_saving = 0;
	char c[1];
	int read_length = fread(c, sizeof(c), 1, file);
	while (read_length == 1){
		if (c[0] == '\n'){
			create_newline(state, state->cursor_y);
		} else {
			insert_character(state, state->cursor_y, state->cursor_x, c[0]);
		}
		read_length = fread(c, sizeof(c), 1, file);
	}
	fclose(file);
}

void write_file(editor_state_t* state){
	if (state->filename_length == 0){ return; }
	const char* filename = state->filename;
	FILE* file = fopen(filename, "w");
	if (!file){ return; }
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

int main(int argc, char* argv[]){
	initialize_ncurses();
	
	static editor_state_t editor_state;
	editor_state.filename_length = 0;
	editor_state.line_count = 1;
	editor_state.lines[0].length = 0;
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
	editor_state.furthest_right = 0;
	
	char last_key = ' ';
	render_all(&editor_state);
	while (1){
		int key = getch();
		editor_state.screen_width = COLS;
		editor_state.screen_height = LINES;
		switch(key){
			case(10):
			case(KEY_ENTER):     { send_special_key(&editor_state, FOX_KEY_ENTER);     break; } 
			case(KEY_BACKSPACE): { send_special_key(&editor_state, FOX_KEY_BACKSPACE); break; }
			case(KEY_DC):        { send_special_key(&editor_state, FOX_KEY_DELETE);    break; }
			case(KEY_LEFT):      { send_special_key(&editor_state, FOX_KEY_LEFT);      break; }
			case(KEY_RIGHT):     { send_special_key(&editor_state, FOX_KEY_RIGHT);     break; }
			case(KEY_UP):        { send_special_key(&editor_state, FOX_KEY_UP);        break; }
			case(KEY_DOWN):      { send_special_key(&editor_state, FOX_KEY_DOWN);      break; }
			case(KEY_HOME):      { send_special_key(&editor_state, FOX_KEY_HOME);      break; }
			case(KEY_END):       { send_special_key(&editor_state, FOX_KEY_END);       break; }
			case(KEY_PPAGE):     { send_special_key(&editor_state, FOX_KEY_PAGE_UP);   break; } // Page up
			case(KEY_NPAGE):     { send_special_key(&editor_state, FOX_KEY_PAGE_DOWN); break; } // Page down
			case(19):            { send_special_key(&editor_state, FOX_KEY_SAVE);      break; } // Save
			case(27):{ break; } // I don't remember what key this is, probably esc
#ifdef KEY_RESIZE
			case(KEY_RESIZE): { break; }
#endif
			default: { type_character(&editor_state, key); break; }
		}
		render_all(&editor_state);
	}
	
	endwin();
	
	return 0;
}
