#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <gtk/gtk.h>


uint8_t * screen_mem;
uint16_t * palette_mem;

// use the pico-8 pattern as a starting point
uint32_t builtin_colors[16] = {
    0x000000, 0xFFFFFF, 0x7E2553, 0x008751,
    0xAB5236, 0x5F574F, 0xC2C3C7, 0xFFF1E8,
    0xFF004D, 0xFFA300, 0xFFEC27, 0x00E436,
    0x29ADFF, 0x83769C, 0xFF77A8, 0xFFCCAA
};

// Cairo drawing stuff
cairo_surface_t * surface;
cairo_pattern_t * pattern;
uint8_t * pixels;
int rowstride;
#define SCALE 3

void draw_pixel(int y, int x, uint32_t color) {
  uint8_t * pixel = &pixels[(y * rowstride) + (x * 4)];
  // no idea yet if this is in the right order
  pixel[0] = color & 0xFF;
  pixel[1] = (color >> 8) & 0xFF;
  pixel[2] = (color >> 16) & 0xFF;
}

void draw_monochrome(uint16_t palette, int i, int j, int sprite_bank, int sprite_idx) {
  uint8_t * bank_mem = &screen_mem[0x0400 + (sprite_bank * 16 * 8)];
  int sprite_x = sprite_idx & 0b0011;
  int sprite_y = (sprite_idx & 0b1100) >> 2;

  for (int row=0; row<8; row++) {
    uint8_t slice = bank_mem[(sprite_y + row) * 4];
    for (int pixel=0; pixel<8; pixel++) {
      int idx = (slice >> (7-pixel)) & 0b1; // a 1-bit index, but we still use it as an index
      uint32_t color = builtin_colors[palette >> (4*idx) & 0b1111];
      draw_pixel(i*8 + row, j*8 + pixel, color);
    }
  }
}

void draw_indexed(uint16_t palette, int i, int j, int sprite_bank, int sprite_idx) {
  uint8_t * bank_mem = &screen_mem[0x0600 + (sprite_bank * 16 * 8)];
  int sprite_x = sprite_idx & 0b0011;
  int sprite_y = (sprite_idx & 0b1100) >> 2;

  for (int row=0; row<8; row++) {
    uint8_t slice = bank_mem[(sprite_y + row) * 4];
    for (int pixel=0; pixel<4; pixel++) {
      int idx = (slice >> ((3-pixel)*2)) & 0b11; // a 2-bit index here
      int color = builtin_colors[palette >> (4*idx) & 0b1111];
      // indexed is 2 pixels wide
      draw_pixel(i*8 + row, j*8 + (pixel*2), color);
      draw_pixel(i*8 + row, j*8 + (pixel*2) + 1, color);
    }
  }
}

/**
 * This draws the screen from screen memory as per the Tricolore specs.
 */
void draw_screen() {
  for(int i=0; i<30; i++) {

    uint16_t * border = (uint16_t *) &screen_mem[(i<<5) | 30]; // find pallete index data at side border

    for (int j=0; j<30; j++) {
      uint8_t block = screen_mem[(i<<5) | j];
      int border_idx = block >> 6;
      int sprite_bank = block >> 4 & 0b11;
      int sprite_idx = block & 0b1111;

      // The 2 most significant bits of 'block' are an index into the border data;
      // The border data then gives the palette index.
      int palette_idx = ((* border) >> border_idx) & 0b1111;
      uint16_t palette = palette_mem[palette_idx];

      if (border_idx == 0) {
        draw_monochrome(palette, i, j, sprite_bank, sprite_idx);
      } else {
        draw_indexed(palette, i, j, sprite_bank, sprite_idx);
      }
    }
  }
}

void configure_cb(GtkWidget * widget, GdkEventConfigure * event, gpointer data) {
  // The surface is 'patterned off' the Gdk Window,
  // but apart from thatis just an off-display buffer.
  // So we can draw it just once here (pending data changes)
  GdkWindow * window = gtk_widget_get_window(widget);
  surface = gdk_window_create_similar_surface(window,
        CAIRO_CONTENT_COLOR,
        gtk_widget_get_allocated_width (widget),
        gtk_widget_get_allocated_height (widget));

  // The derived 'pattern' object may also be set up once
  pattern = cairo_pattern_create_for_surface(surface);
  cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);

  pixels = cairo_image_surface_get_data(surface);
  rowstride = cairo_image_surface_get_stride(surface);
}

void draw_cb(GtkWidget *widget, cairo_t * cr, gpointer userdata) {
  if (!(cairo_in_clip(cr, 0,0) || cairo_in_clip(cr, 239, 239))) return; // not (presently) drawing anything outside this area

  draw_screen();
  cairo_surface_mark_dirty(surface);

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_scale(cr, SCALE, SCALE);
  cairo_set_source(cr, pattern);
  cairo_paint(cr);
}

void delete_cb(GtkWidget *widget, GdkEventType *event, gpointer userdata) {
  // destroy any global drawing state here,
  // like the cairo surface (if we make it global)
  cairo_surface_destroy(surface);
  gtk_main_quit();
}

void display_runloop(int argc, char ** argv) {
  gtk_init (&argc, &argv);

  GtkWindow * window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title(window, "pixbuf");
  gtk_window_set_default_size(window, 400*SCALE, 300*SCALE);
  gtk_window_set_resizable(window, FALSE);

  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(delete_cb), NULL);

  GtkWidget * drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, 32,8);
  gtk_container_add(GTK_CONTAINER(window), drawing_area);
  g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(draw_cb), NULL);
  g_signal_connect(drawing_area,"configure-event", G_CALLBACK (configure_cb), NULL);

  gtk_widget_show_all(GTK_WIDGET(window));
  gtk_main();
}

void paint_something() {
	screen_mem[0] = 0; // monochrome sprite #0; note other zero'd boxes will also show this
	screen_mem[1] = 0b01000000; // colour sprite #0
	screen_mem[30] = 0x00; // select some palettes
	screen_mem[31] = 0x32; // in thingy endian
	screen_mem[62] = 0x01; // line 2, different palette
	screen_mem[63] = 0x23; // in thingy endian
	// sprite #0: something with clear orientation
	screen_mem[0x0400] = 0b00000000;
	screen_mem[0x0404] = 0b00111100;
	screen_mem[0x0408] = 0b01100110;
	screen_mem[0x040C] = 0b01100110;
	screen_mem[0x0410] = 0b01100110;
	screen_mem[0x0414] = 0b01101110;
	screen_mem[0x0418] = 0b00111111;
	screen_mem[0x041C] = 0b00000000;
	// color sprite #0: something something
	screen_mem[0x0600] = 0x05;
	screen_mem[0x0604] = 0x05;
	screen_mem[0x0608] = 0x05;
	screen_mem[0x060C] = 0x05;
	screen_mem[0x0610] = 0xAF;
	screen_mem[0x0614] = 0xAF;
	screen_mem[0x0618] = 0xAF;
	screen_mem[0x061C] = 0xAF;
	// palette #0: select any 4 colors (thingy endian)
	palette_mem[0] = 0x3210;
	palette_mem[1] = 0x4321;
}

int main (int argc, char ** argv) {
  screen_mem = malloc(sizeof(uint8_t) * 0x800);
  palette_mem = (uint16_t *) &screen_mem[0x03C0];

  paint_something();
  display_runloop(argc, argv);
  return 0;
}
