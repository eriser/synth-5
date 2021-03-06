// fontengine
// Copyright James Higgs 2009

class FontEngine
{
public:
	// constructor
	FontEngine(SDL_Renderer* renderer, const char* fontfile, int font_char_width, int font_char_height);
	
	// operations
//	void DrawGlyph(SDL_Surface* surface, char c, int destx, int desty);
//	void DrawText(SDL_Surface* surface, const char *s, const SDL_Rect& rect, bool clip);
	void DrawGlyph(SDL_Renderer* renderer, char c, int destx, int desty);
	void DrawText(SDL_Renderer* renderer, const char *s, const SDL_Rect& rect, bool clip);
	int GetFontHeight() { return fontCharHeight; }
	int GetFontWidth() { return fontCharWidth; }
	
private:	
	//SDL_Surface *fontImg;
	SDL_Texture *m_fontTexture;
	int fontCharsPerline;
	int fontCharWidth;
	int fontCharHeight;
};
