#pragma once

class GIMPBMP{
public:
  UINT16 cx;
  UINT16 cy;
  UINT8  bytes_per_pixel;
  UINT8* pixel_data;
  UINT8* GetPixelAddr(int x, int y) const {  return &pixel_data[ (y*cx + x)*bytes_per_pixel ]; }
};

extern GIMPBMP contactbook;
extern GIMPBMP contactbook_add;
extern GIMPBMP contactbook_up;
extern GIMPBMP frame1_l;
extern GIMPBMP frame1_r;
extern GIMPBMP icon_id;
extern GIMPBMP icon_ip;
extern GIMPBMP indicator01_a;
extern GIMPBMP indicator01_a_v1;
extern GIMPBMP indicator01_a_v2;
extern GIMPBMP indicator01_b;
extern GIMPBMP mainwnd_vert_delimiter;
