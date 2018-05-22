/** New Conformist
    Copyright (C) 2018 Zeno Rogue

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "graph2.h"
#include <unistd.h>
#include <unordered_map>
#include <array>
#include <vector>
#include <map>
#include <cstdio>
#include <cmath>
#include <string>
#include <algorithm>

using namespace std;

#define MAXSIDE 16

#include "mat.cpp"
#include "zebra.cpp"

int sides = 1;

bool draw_progress = true;

bitmap heart;

struct datapoint {
  cpoint x;
  int type;
  int state;
  int side=0;
  ld bonus;
  unordered_map<datapoint*, ld> eqs;
  };

vector<vector<datapoint>> pts;

int SX, SY;

int dx[4] = {1, 0, -1, 0};
int dy[4] = {0, -1, 0, 1};

ld scalex = 1, scaley = 1;
int marginx = 32, marginy = 32;

void resize_pt() {
  pts.resize(SY);
  for(int y=0; y<SY; y++) pts[y].resize(SX);
  }

void createb_rectangle() {
  sides = 1;
  resize_pt();
  for(int y=0; y<SY; y++) 
  for(int x=0; x<SX; x++) {
    auto& p = pts[y][x];
    p.type = 1;
    p.side = 0;
    if(y == 0 || y == SY-1 || x == 0 || x == SX-1) {
      if(y < SY/2) p.type = 4;
      else if(y > SY/2) p.type = 5;
      else if(x == 0) p.type = 6;
      else p.type = 7;
      }
    }
  }

void createb(bool inner, const string& fname) {
  sides = 1;
  heart = readPng(fname.c_str());
  errpixel = heart[0][0];
  
  int newSX = heart.s->w / scalex + marginx + marginx;
  if(newSX < SX) 
    marginx = (SX - heart.s->w / scalex) / 2;
  else SX = newSX;

  int newSY = heart.s->h / scaley + marginy + marginy;
  if(newSY < SY) 
    marginy = (SY - heart.s->h / scaley) / 2;
  else SY = newSY;

  resize_pt();
  sidetype[0] = inner ? 0 : 1;

  for(int y=0; y<SY; y++) 
  for(int x=0; x<SX; x++) {
    auto& p = pts[y][x];
    p.side = 0;
    if(inner) {
      if(heart[(y-marginy)*scaley][(x-marginx)*scalex] == errpixel)
        p.type = 0;
      else
        p.type = 1;
      }
    else {
      if(x < 1 || y < 1 || x == SX-1 || y == SY-1)
        p.type = 4;
      else if(heart[(y-marginy)*scaley][(x-marginx)*scalex] != errpixel)
        p.type = 5;
      else if(x > SX/2)
        p.type = 1;
      else if(y < SY/2)
        p.type = 2;
      else
        p.type = 3;
      }
    }
  
  if(inner) {
    for(int x=0; x<SX; x++) for(int y=0; y<SY; y++)
      if(pts[y][x].type == 0 && pts[y][x+1].type == 1) {
        pts[y][x].type = 6;
        goto bxy;
        }
  
    bxy:
    int bx, by;
    bool first = true;
    for(int x=1; x<SX; x++) for(int y=0; y<SY; y++)
      if(pts[y][x].type == 0 && pts[y][x-1].type == 1 && (first || x-y > bx-by)) {
        bx = x, by = y;
        first = false;
        }
    pts[by][bx].type = 7;
    bx--; int d = 0;
    
    int phase = 5;
    
    for(int iter=0; iter<100000; iter++) {
      d &= 3;
      auto& pt2 = pts[by+dy[d]][bx+dx[d]];
      if(pt2.type == phase+2 || pt2.type == phase) d++;
      else if(pt2.type == 0) { pt2.type = phase; d++; }
      else if(pt2.type == 6 || pt2.type == 7) { phase--; if(phase == 3) break; }
      else if(pt2.type == 1) { by += dy[d]; bx += dx[d]; d--; }
      }

    }
  }

void saveb(const string& s) {  
  FILE *f = fopen(s.c_str(), "wt");
  printf("%d %d\n", SX, SY);
  for(int y=0; y<SY; y++) {
    for(int x=0; x<SX; x++)
      printf("%c", "X.-+TDLR" [pts[y][x].type]);
    printf("\n");
    }
  fclose(f);
  }

int pointorder(pair<int, int> xy) {
  int x = xy.first, y = xy.second;
  if(x==0 && y==0) return -1;
  if(x%2 == 0 || y%2 == 0)
    return pointorder({y, (x&1) ? 1 : (x >> 1)}) + 1;
  else
    return 0;
  }

void drawstates() {
  if(!draw_progress) return;
  initGraph(SX, SY, "conformist", false);
  int statecolors[4] = {
    0, 0xFF0000, 0x00FF00, 0x0000FF };

  for(int y=0; y<SY; y++)
  for(int x=0; x<SX; x++) {
    auto& p = pts[y][x];
    screen[y][x] = statecolors[p.state];
    }
  screen.draw();
  
  }

void computemap() {

  for(int i=0; i<2; i++) {
    printf("Building eqs, i=%d\n", i);

    for(int y=0; y<SY; y++) 
    for(int x=0; x<SX; x++) {
      auto& p = pts[y][x];
      p.state = 0;
      if(p.type >= 4 || p.type == 0) continue;
      p.state = 1;
      p.eqs.clear();
      p.bonus = 0;

      p.x[i] = 0;
      p.bonus = 0;

      if(i == 0) {
        int xp = 0;
        for(int d=0; d<4; d++) {
          auto t = pts[y+dy[d]][x+dx[d]].type;
          if(t < 4 || t == 6 || t == 7) xp++;
          }
        for(int d=0; d<4; d++) {
          auto& p2 = pts[y+dy[d]][x+dx[d]];
          if(p2.type == 6) p.bonus += 0;
          else if(p2.type == 7) p.bonus += 1./xp;
          else if(p2.type < 4) {
            p.eqs[&p2] = 1./xp;
            if(p.type == 2 && p2.type == 3) p.bonus += 1./xp;
            if(p.type == 3 && p2.type == 2) p.bonus -= 1./xp;
            }
          }
        }
      if(i == 1) {
        for(int d=0; d<4; d++) {
          auto& p2 = pts[y+dy[d]][x+dx[d]];
          if(p2.type == 4) p.bonus += 0; 
          else if(p2.type == 5) p.bonus += 1./4; 
          else if(p2.type == 6 || p2.type == 7) p.bonus += 1./8;
          else p.eqs[&p2] = 1./4;
          }
        }
      }
    
    vector<pair<int, int> > allpoints;
    for(int y=0; y<SY; y++) 
    for(int x=0; x<SX; x++) if(pts[y][x].state == 1) allpoints.push_back({x, y});
    sort(allpoints.begin(), allpoints.end(), [] (auto p1, auto p2) { return pointorder(p1) < pointorder(p2); });

    printf("Gaussian elimination\n");
    int lastpct = -1, citer = 0;
    for(auto co: allpoints) {
      auto &p = pts[co.second][co.first];
      if(p.state != 1) continue;
      int cpct = citer * 100 / size(allpoints);
      if(cpct != lastpct) {
        lastpct = cpct;
        printf("  %d%% [%d]\n", cpct, size(p.eqs));
        drawstates();
        }
      citer++;
      if(p.eqs.count(&p)) {
        if(p.eqs[&p] == 1) {
          printf("Variable eliminated at (%d,%d)\n", co.first, co.second);
          p.state = 3;
          }
        else {
          ld fac = 1 / (1 - p.eqs[&p]);
          p.eqs.erase(&p);
          for(auto& pa: p.eqs) pa.second *= fac;
          p.bonus *= fac;
          }
        }
      for(auto& pa: p.eqs) {
        auto& p2 = *pa.first;
        ld mirror = p2.eqs[&p];
        p2.eqs.erase(&p);
        p2.bonus += p.bonus * mirror;
        for(auto& pa: p.eqs) p2.eqs[pa.first] += pa.second * mirror;
        }
      p.state = 2;
      }
    
    printf("Solution retrieval\n");
    reverse(allpoints.begin(), allpoints.end());
    for(auto co: allpoints) {
      auto &p = pts[co.second][co.first];
      if(p.state != 2) continue;
      p.x[i] = p.bonus;
      for(auto& pa: p.eqs) p.x[i] += pa.second * pa.first->x[i];
      }
    
    printf("Done.\n");        
    }
  }

int mousex, mousey;

void savemap(const string& fname) {
  FILE *f = fopen(fname.c_str(), "wb");
  if(!f) pdie("savemap");
  fwrite(&SX, sizeof(SX), 1, f);
  fwrite(&SY, sizeof(SY), 1, f);
  for(int y=0; y<SY; y++)
  for(int x=0; x<SX; x++) {
    auto& p = pts[y][x];
    fwrite(&p.x, sizeof(p.x), 1, f);
    fwrite(&p.type, sizeof(p.type), 1, f);
    }
  fclose(f);
  }

void loadmap(const string& fname) {
  sides = 1;
  sidetype[0] = 0;
  FILE *f = fopen(fname.c_str(), "rb");
  if(!f) pdie("loadmap");
  fread(&SX, sizeof(SX), 1, f);
  fread(&SY, sizeof(SY), 1, f);
  resize_pt();
  for(int y=0; y<SY; y++)
  for(int x=0; x<SX; x++) {
    auto& p = pts[y][x];
    fread(&p.x, sizeof(p.x), 1, f);
    fread(&p.type, sizeof(p.type), 1, f);
    if(p.type == 2) sidetype[0] = 1;
    p.side = 0;
    }
  fclose(f);
  }

void loadmap2(const string& fname) {
  sidetype[sides] = 0;
  FILE *f = fopen(fname.c_str(), "rb");
  if(!f) pdie("loadmap2");
  int iSX, iSY;
  fread(&iSX, sizeof(iSX), 1, f);
  fread(&iSY, sizeof(iSY), 1, f);
  if(iSX != SX || iSY != SY) die("map size mismatch\n");
  for(int y=0; y<SY; y++)
  for(int x=0; x<SX; x++) {
    datapoint dp; 
    fread(&dp.x, sizeof(dp.x), 1, f);
    fread(&dp.type, sizeof(dp.type), 1, f);
    if(dp.type < 4 && dp.type > 0) {
      dp.side = sides;
      pts[y][x] = dp;
      if(dp.type == 2) sidetype[sides] = 1;
      }
    }
  fclose(f);
  sides++;
  }

// how should be linearly transform the current harmonic mapping to make it conformal
// ([1] should be 0 and is ignored, we only have to scale the x coordinate by multiplying
// by get_conformity(...)[0]))
cpoint get_conformity(int x, int y) {
  
  auto vzero = pts[y][x].x;
  array<cpoint, 2> v = {pts[y][x+1].x - vzero, pts[y+1][x].x - vzero };
  
  ld det = v[0] ^ v[1];
  
  cpoint ba2 = cpoint{v[1][1], -v[0][1]} / det;
  cpoint ca2 = cpoint{-v[1][0], v[0][0]} / det;
  
  ld bad = (ca2|ba2) / (ba2|ba2);
  ld good = (ca2^ba2) / (ba2|ba2);
  
  return cpoint{good, bad};
  }  

void draw(bitmap &b) {
  b.belocked();
  for(int y=0; y<SY; y++)
  for(int x=0; x<SX; x++) {
    auto& p = pts[y][x];
    if(p.type == 0) {
      b[y][x] = 0xFFD500;
      continue;
      }
    
    int si = p.side;
    if(img_band[si].size()) {
      ld by = p.x[1];
      ld bx = p.x[0] - xcenter[si];
      bx /= cscale[si][0];
      int sizy = img_band[si][0].s->h;
      by *= sizy;
      bx *= sizy;
      int totalx = 0;
      for(auto& bandimg: img_band[si]) totalx += bandimg.s->w;
      bx += totalx/2.;
      bx -= int(bx) / totalx * totalx;
      if(bx < 0) bx += totalx;
      for(auto& bandimg: img_band[si]) { 
        if(bx < bandimg.s->w) {
          b[y][x] = bandimg[by][bx];
          break;
          }
        else bx -= bandimg.s->w;
        }      
      }
    else if(!img[si].s) {
      b[y][x] = int(255 & int(256 * pts[y][x].x[0])) + ((255 & int(255 * pts[y][x].x[1])) << 8) + (sides>1 ? (((si * 255) / (sides-1)) << 16) : 0);
      }
    else {
      auto dc = band_to_disk(p.x, si);
      b[y][x] = img[si][dc[1]][dc[0]];
      /*
      LOAD BAND
      auto by = int(p.x[1] * imgb.s->h);
      auto bx = int(p.x[0] / cscale[0] * imgb.s->h);
      bx %= imgb.s->w;
      if(bx<0) bx += imgb.s->w;
      
      b[y][x] = imgb[by][bx];
      */
      }
    }
  b.draw();
  }

void klawisze() {
  SDL_Event event;
  SDL_Delay(1);
  int ev;
  while(ev = SDL_PollEvent(&event)) switch (event.type) {
    case SDL_QUIT:
      exit(1);
      return;

    case SDL_MOUSEBUTTONDOWN: {
      break;
      }
    
    case SDL_MOUSEMOTION: {
      mousex = event.motion.x;
      mousey = event.motion.y;
      break;
      }
    
    case SDL_KEYDOWN: {
      int key = event.key.keysym.sym;
      int uni = event.key.keysym.unicode;
      
      if(key == 'q') exit(1);
      
      break;
      }
    
    }
  }

bool inner(int t) { return t > 0 && t< 4; }

void load_image(const string& fname) {
  img[sides-1] = readPng(fname); 
  }

void load_image_band(const string& fname) {
  img_band[sides-1].push_back(readPng(fname));
  }

void measure(int si) {
  vector<ld> cscs[2];
  
  printf("side #%d, type %d\n", si, sidetype[si]);
  
  for(int y=0; y<SY; y++)
  for(int x=0; x<SX; x++) if(pts[y][x].side == si) if(inner(pts[y][x].type) && pts[y+1][x].side == si && inner(pts[y+1][x].type) && pts[y][x+1].side == si && inner(pts[y][x+1].type)) {
    auto c = get_conformity(x, y);
    for(int i: {0,1}) cscs[i].push_back(c[i]);
    }
  
  for(int i: {0,1}) sort(cscs[i].begin(), cscs[i].end());  
  int q = size(cscs[0]);
  
  cscale[si] = { cscs[0][q/2], cscs[1][q/2] };
  
  for(int i=0; i<=16; i++) {
    int id = (q * i) / 16;
    printf("%Lf %Lf\n", cscs[0][id], cscs[1][id]);
    }

  printf("conformity: %Lf %Lf (%d points)\n", cscale[si][0], cscale[si][1], q);
  
  vector<ld> xes;
  for(int y=0; y<SY; y++)
  for(int x=0; x<SX; x++) if(pts[y][x].side == si) if(inner(pts[y][x].type)) xes.push_back(pts[y][x].x[0]);
  sort(xes.begin(), xes.end());
  xcenter[si] = xes[size(xes) / 2];
  printf("xcenter: %Lf\n", xcenter[si]);

  if(sidetype[si] && period[si] > 0) {
    printf("period multiple: %Lf\n", M_PI / cscale[si][0] / period[si]);
  
    if(sidetype[si] == 2) {
      ld pmul = M_PI / cscale[si][0] / period[si];
      pmul = int(pmul + .5);
      cscale[si][0] = M_PI / period[si] / pmul;
      printf("fixed period multiple: %Lf\n", M_PI / cscale[si][0] / period[si]);    
      }
    }
  }

void ui() {
  for(int si=0; si<sides; si++) measure(si);  
  initGraph(SX, SY, "conformist", false);
  while(true) {
    draw(screen);
    klawisze();
    }
  }

void export_image(const string& fname) {
  for(int si=0; si<sides; si++) measure(si);
  bitmap b = emptyBitmap(SX, SY);
  draw(b);
  for(int y=0; y<SY; y++)
  for(int x=0; x<SX; x++)
    b[y][x] |= 0xFF000000;
  writePng(fname.c_str(), b);
  }

int main(int argc, char **argv) {
  for(int i=0; i<MAXSIDE; i++)
    period_unit[i] = 1;
  int i = 1;
  auto next_arg = [&] () { if(i == argc) die("not enough arguments"); return argv[i++]; };
  while(i < argc) {
    string s = next_arg();
    if(s == "-scale") scalex = scaley = atoi(next_arg());
    else if(s == "-margin") marginx = marginy = atoi(next_arg());
    else if(s == "-rectangle") {
      SX = atoi(next_arg());
      SY = atoi(next_arg());
      createb_rectangle();
      }
    else if(s == "-cbo") createb(false, next_arg());
    else if(s == "-cbi") createb(true, next_arg());
    else if(s == "-sb") saveb(next_arg());
    else if(s == "-q") draw_progress = false;
    else if(s == "-cm") computemap();
    else if(s == "-sm") savemap(next_arg());
    else if(s == "-lm") loadmap(next_arg());
    else if(s == "-lm2") loadmap2(next_arg());
    else if(s == "-li") load_image(next_arg());
    else if(s == "-lband") load_image_band(next_arg());
    else if(s == "-lbands") {
      int i0 = atoi(next_arg());
      int i1 = atoi(next_arg());
      string s = next_arg();
      for(int i=i0; i<=i1; i++) {
        char buf[100000];
        snprintf(buf, 100000, s.c_str(), i);
        printf("Loading image #%d: %s\n", i, buf);
        load_image_band(buf);
        }
      }
    else if(s == "-zebra") period_unit[sides-1] = zebra_period, period_matrices[sides-1] = zebra_matrices;
    else if(s == "-period") period[sides-1] = period_unit[sides-1] * atoi(next_arg());
    else if(s == "-fix") sidetype[sides-1] = 2;
    else if(s == "-draw") ui();
    else if(s == "-export") export_image(next_arg());
    else die("unrecognized argument\n");
    }

  return 0;
  }
