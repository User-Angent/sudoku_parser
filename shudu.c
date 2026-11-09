#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

typedef struct {
	uint8_t is_certain: 1;
	uint8_t certain: 4;
	uint16_t poss: 9;
} block_t;

void to_block(uint8_t * raw, block_t * blocks) {
	for (int i = 0; i < 81; ++i) {
		if (raw[i]) {
			blocks[i].is_certain = 1;
			blocks[i].certain = raw[i];
			blocks[i].poss = 0;
		} else {
			blocks[i].is_certain = 0;
			blocks[i].certain = 0;
			blocks[i].poss = (1 << 9) - 1;
		}
	}
}

void to_array(block_t * blocks, uint8_t * dst) {
	for (int i = 0; i < 81; ++i)
		if (blocks[i].is_certain) dst[i] = blocks[i].certain;
		else dst[i] = 0;
}

int blposs(block_t bl) {
	int poss = 0;
	for (int i = 0; i < 9; ++i)
		if (bl.poss & (1 << i))
			++poss;
	return poss;
}

void blcopy(block_t * dst, const block_t * src) {
	for (int i = 0; i < 81; ++i)
		dst[i] = src[i];
}

void check_row(block_t * blocks, int pos) {
	int row = pos / 9;
	uint16_t mask = 1 << (blocks[pos].certain - 1);
	for (int i = 0; i < 9; ++i)
		blocks[row * 9 + i].poss &= ~mask;
}

void check_col(block_t * blocks, int pos) {
	int col = pos % 9;
	uint16_t mask = 1 << (blocks[pos].certain - 1);
	for (int i = 0; i < 9; ++i)
		blocks[i * 9 + col].poss &= ~mask;
}

void check_nsg(block_t * blocks, int pos) {
	int srow = (pos / 27) * 3, scol = (pos % 9 / 3) * 3;
	uint16_t mask = 1 << (blocks[pos].certain - 1);
	for (int i = 0; i < 9; ++i) {
		int index  = (srow + i / 3) * 9 + scol + i % 3;
		blocks[index].poss &= ~mask;
	}
}

int check_row_valid(block_t *blocks) {
	block_t (*bl)[9] = (block_t (*)[9])blocks;
	for (int row = 0; row < 9; ++row) {
		uint16_t poss = (1 << 9) - 1;
		for (int col = 0; col < 9; ++col) {
			if (!bl[row][col].is_certain)
				continue;
			uint16_t mask = 1 << (bl[row][col].certain - 1);
			if (mask & poss) poss &= ~mask;
			else return -1;
		}
	}
	return 0;
}

int check_col_valid(block_t *blocks) {
	block_t (*bl)[9] = (block_t (*)[9])blocks;
	for (int col = 0; col < 9; ++col) {
		uint16_t poss = (1 << 9) - 1;
		for (int row = 0; row < 9; ++row) {
			if (!bl[row][col].is_certain)
				continue;
			uint16_t mask = 1 << (bl[row][col].certain - 1);
			if (mask & poss) poss &= ~mask;
			else return -1;
		}
	}
	return check_row_valid(blocks);
}

int check_nsg_valid(block_t *blocks) {
	for (int nsg = 0; nsg < 9; ++nsg) {
		uint16_t poss = (1 << 9) - 1;
		int srow = (nsg / 3) * 3, scol = (nsg % 3) * 3;
		for (int i = 0; i < 9; ++i) {
			int idx = (srow + i / 3) * 9 + scol + i % 3;
			if (!blocks[idx].is_certain)
				continue;
			uint16_t mask = 1 << (blocks[idx].certain - 1);
			if (mask & poss) poss &= ~mask;
			else return -1;
		}
	}
	return check_col_valid(blocks);
}

int check_valid(block_t * blocks) {
	for (int i = 0; i < 81; ++i) {
		if (blocks[i].is_certain)
			continue;
		if (blposs(blocks[i]) == 0)
			return -1;
	}
	return check_nsg_valid(blocks);
}

int check_compl(block_t * blocks) {
	for (int i = 0; i < 81; ++i)
		if (!blocks[i].is_certain)
			return 0;
	return 1;
}

int update_blocks(block_t * blocks) {
	int is_changed = 0;
	for (int i = 0; i < 81; ++i) {
		if (blocks[i].is_certain)
			continue;
		if (blposs(blocks[i]) != 1)
			continue;
		blocks[i].is_certain = 1;
		is_changed = 1;
		for (int k = 0; k < 9; ++k)
			if (blocks[i].poss & (1 << k)) {
				blocks[i].certain = k + 1;
				break;
			}
	}
	return is_changed;
}

int guess(block_t * blocks) {
	int decode(block_t *);
	for (int poss = 2; poss < 10; ++poss) {
		for (int i = 0; i < 81; ++i) {
			if (blocks[i].is_certain)
				continue;
			if (blposs(blocks[i]) > poss)
				continue;
			for (int n = 0; n < 9; ++n) {
				if (blocks[i].poss & (1 << n)) {
					block_t new_bl[81];
					blcopy(new_bl, blocks);
					new_bl[i].is_certain = 1;
					new_bl[i].certain = n + 1;
					if (decode(new_bl) == 0) {
						blcopy(blocks, new_bl);
						return 0;
					}
				}
			}
			return -1;
		}
	}
	return -1;
}

int decode(block_t * blocks) {
	for (int none = 0; none < 32; ) {
		for (int i = 0; i < 81; ++i) {
			if (!blocks[i].is_certain)
				continue;
			check_row(blocks, i);
			check_col(blocks, i);
			check_nsg(blocks, i);
		}
		if (update_blocks(blocks))
			none = 0;
		else ++none;
		
		if (check_valid(blocks) < 0)
			return -1;
	}
	if (check_compl(blocks))
		return 0;
	return guess(blocks);
}

int main(int argc, char *argv[]) {
	FILE * pro_out = fopen("program-log.stdout", "w");
	SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0) {
		fprintf(pro_out, "SDL_Init error: %s\n", SDL_GetError());
		return -1;
	}
	SDL_Window *window = SDL_CreateWindow("数独解析器", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1080, 1800, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
	if (window == NULL) {
		fprintf(pro_out, "SDL_CreateWindow error: %s\n", SDL_GetError());
		return -1;
	}
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		fprintf(pro_out, "SDL_CreateRenderer error: %s\n", SDL_GetError());
		return -1;
	}
	if (TTF_Init() < 0) {
		fprintf(pro_out, "TTF_Init error: %s\n", SDL_GetError());
		return -1;
	}
	
	TTF_Font *font = TTF_OpenFont("/system/fonts/DroidSansChinese.ttf", 32);
	TTF_Font *fnum = TTF_OpenFont("/system/fonts/SmartZawgyi.ttf", 96);
	SDL_RenderSetLogicalSize(renderer, 1080, 1800);
	
	uint8_t layout[9][9], parsing = 0;
	memset(layout, 0, sizeof(layout));
	struct { uint8_t valid, x, y; } choosed = {0};
	for (uint8_t running = 1; running; ) {
		uint32_t st_tick = SDL_GetTicks();
		for (SDL_Event e; SDL_PollEvent(&e);) {
			if (e.type == SDL_QUIT) {
				running = 0;
				break;
			} else if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.scancode == SDL_SCANCODE_AC_BACK) {
					running = 0;
					break;
				}
			} else if (e.type == SDL_FINGERDOWN) {
				if (parsing == 1) continue;
				int x = (int)roundf(e.tfinger.x * 1080.0F);
				int y = (int)roundf(e.tfinger.y * 1800.0F);
				if (y >= 90 && y < 990 && x >= 90 && x < 990) {
					choosed.valid = 1;
					choosed.x = (x - 90) / 100;
					choosed.y = (y - 90) / 100;
				} else if (y >= 1200 && y < 1520 && x >= 140 && x < 940) {
					if (choosed.valid) {
						uint8_t *p = &(layout[choosed.y][choosed.x]);
						if ((*p = 1 + ((x - 140) / 160) + ((y - 1200) / 160) * 5) == 10) *p = 0;
						if (*p == 0 && parsing == 255) memset(layout, 0, sizeof(layout)), parsing = 0;
					}
				} else if (y >= 15 && y < 75 && x >= 90 && x < 330)
					parsing = 1;
				// printf("(%d, %d), ", x, y);
			}
		}
		SDL_SetRenderDrawColor(renderer, 0x3A, 0x41, 0x6B, 255);
		SDL_RenderClear(renderer);
		{
			SDL_Rect rect = {90, 90, 900, 900};
			SDL_SetRenderDrawColor(renderer, 0xEF, 0xEF, 0xEF, 0xFF);
			SDL_RenderFillRect(renderer, &rect);
		}
		SDL_SetRenderDrawColor(renderer, 0x19, 0x1C, 0x29, 255);
		for (int xy = 0; xy <= 9; ++xy) {
			int width = (1 + (xy % 3 == 0)) * 2;
			for (int i = -(width-1); i <= width; ++i) {
				SDL_RenderDrawLine(renderer, 90 + 100 * xy + i, 88, 90 + 100 * xy + i, 993);
				SDL_RenderDrawLine(renderer, 88, 90 + 100 * xy + i, 993, 90 + 100 * xy + i);
			}
		}
		for (int x = 0; x < 5; ++x) {
			for (int y = 0; y < 2; ++y) {
				const int tab = 16;
				SDL_Rect rect = { 140 + x * 160 + tab, 1200 + y * 160 + tab, 160 - tab * 2, 160 - tab * 2 };
				SDL_SetRenderDrawColor(renderer, 0x5A, 0x61, 0x5A, 255);
				SDL_RenderFillRect(renderer, &rect);
				for (int i = 0; i < 4; ++i) {
					SDL_Rect rt = rect;
					rt.x += i; rt.y += i; rt.w -= i * 2; rt.h -= i * 2;
					SDL_SetRenderDrawColor(renderer, 0x3A, 0x41, 0x6B, 256 * (3 - i));
					SDL_RenderDrawRect(renderer, &rt);
				}
				
				char buffer[2] = {'1' + y * 5 + x, 0};
				buffer[0] = buffer[0] != ':' ? buffer[0] : '-';
				SDL_Surface *src = TTF_RenderText_Solid(fnum, buffer, (SDL_Color){0xEF, 0xEF, 0xEF, 255});
				SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, src);
				SDL_Rect dst = { rect.x + (rect.w - src->w) / 2, rect.y + (rect.h - src->h) / 2, src->w, src->h };
				SDL_RenderCopy(renderer, tex, NULL, &dst);
				SDL_DestroyTexture(tex);
				SDL_FreeSurface(src);
			}
		}
		if (choosed.valid) {
			SDL_Rect rect = { 93 + 100 * choosed.x, 93 + 100 * choosed.y, 96, 96 };
			for (int i = 0; i < 4; ++i) {
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 256 * (3 - i));
				SDL_RenderDrawRect(renderer, &rect);
				++rect.x; ++rect.y; rect.w -= 2; rect.h -= 2;
			}
		}
		for (int y = 0; y < 9; ++y) {
			for (int x = 0; x < 9; ++x) {
				if (!layout[y][x]) continue;
				char buffer[2] = {'0' + layout[y][x], 0};
				SDL_Surface *src = TTF_RenderText_Solid(fnum, buffer, (SDL_Color){0x19, 0x1C, 0x29, 255});
				SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, src);
				SDL_Rect rect = {140 - src->w / 2 + 100 * x, 90 + 100 * y, src->w * 100 / src->h, 100};
				SDL_RenderCopy(renderer, tex, NULL, &rect);
				SDL_DestroyTexture(tex);
				SDL_FreeSurface(src);
			}
		}
		{
			SDL_Surface * src = TTF_RenderUTF8_Solid(font, "开始解析", (SDL_Color){0xEF, 0xEF, 0xEF, 0xFF});
			SDL_Texture * tex = SDL_CreateTextureFromSurface(renderer, src);
			SDL_Rect rect = {90, 15, src->w, src->h};
			SDL_RenderCopy(renderer, tex, NULL, &rect);
			SDL_DestroyTexture(tex);
			SDL_FreeSurface(src);
		}
		if (parsing == 1) {
			SDL_Rect rect = {400, 800, 400, 280};
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 33);
			SDL_RenderFillRect(renderer, &rect);
		}
		SDL_RenderPresent(renderer);
		if (parsing == 1) {
			block_t blocks[81];
			to_block((uint8_t *)layout, blocks);
			if (decode(blocks) == 0)
				to_array(blocks, (uint8_t *)layout);
			parsing = 255;
		}
		uint32_t interval = SDL_GetTicks() - st_tick;
		if (interval < 1000 / 120)
			SDL_Delay(1000 / 120 - interval);
	}
	
	TTF_CloseFont(fnum);
	TTF_CloseFont(font);
	TTF_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	fclose(pro_out);
	return 0;
}