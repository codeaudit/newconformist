ld cspin;

pair<ld, ld> unband(cpoint& c, sideinfo& si, ld shift) {
  ld y = c[1];
  ld x = c[0] + shift;
  
  y *= 2; y -= 1; // -1 .. 1
  x *= 2; x /= si.cscale[0];

  y *= M_PI / 2;
  x *= M_PI / 2;

  y = 2 * atanh(tan(y/2));
  
  return make_pair(x, y);
  }

hyperpoint equirectangular(ld x, ld y) {
  return { sinh(x) * cosh(y), sinh(y), cosh(y) * cosh(x)};
  }    

void set_column(transmatrix& M, int id, const hyperpoint& h) { 
  M[id] = h[0];
  M[id+3] = h[1];
  M[id+6] = h[2];
  }

ld det(const transmatrix& T) {
  ld det = 0;
  for(int i=0; i<3; i++) 
    det += T[i] * T[3+(i+1)%3] * T[6+(i+2)%3];
  for(int i=0; i<3; i++) 
    det -= T[i] * T[3+(i+2)%3] * T[6+(i+1)%3];
  return det;
  }

transmatrix inverse(const transmatrix& T) {  
  ld d = det(T);
  transmatrix T2;
  
  for(int i=0; i<3; i++) 
  for(int j=0; j<3; j++)
    T2[j*3+i] = (T[(i+1)%3*3+(j+1)%3] * T[(i+2)%3*3+(j+2)%3] - T[(i+1)%3*3+(j+2)%3] * T[(i+2)%3*3+(j+1)%3]) / d;

  return T2;
  }

int debugsi;

bool need_btd;

transmatrix get_matrix_at(sideinfo& si, ld x) {
  x -= si.zero_shift;
  int x0 = int(x);
  while(isize(si.matrixlist) <= x0) {
    transmatrix M = mul(si.matrixlist.back(), xpush(1));
    fixmatrix(M);
    M = reperiod(M, rootof(si).period_matrices);
    si.matrixlist.push_back(M);
    }
  return mul(si.matrixlist[x0], xpush(x - x0));
  }

void construct_btd() {
  for(auto& si: sides) {
    
    si.matrixlist.clear();

    if(si.parentid == si.id) {
      si.need_btd = cspin > 0;
      si.matrixlist.push_back(spin(cspin));
      si.zero_shift = -si.animshift;
      }
    
    else {      
      auto& root = rootof(si);
      auto& par = sides[si.parentid];
      par.need_btd = true;
      
      int ex = si.join_x, ey = si.join_y;
      
      auto& ppts = *par.submap;
  
      auto [old_x0, old_y0] = unband(ppts[ey][ex].x, par, 0);
      auto [old_x1, old_y1] = unband(ppts[ey][ex+1].x, par, 0);
      auto [old_x2, old_y2] = unband(ppts[ey+1][ex].x, par, 0);
      
      auto& epts = *si.submap;
  
      auto [new_x0, new_y0] = unband(epts[ey][ex].x, si, 0);
      auto [new_x1, new_y1] = unband(epts[ey][ex+1].x, si, 0);
      auto [new_x2, new_y2] = unband(epts[ey+1][ex].x, si, 0);
      
      transmatrix T = get_matrix_at(par, old_x0);
      
      transmatrix mold, mnew;
      set_column(mold, 0, equirectangular(0, old_y0));
      set_column(mold, 1, equirectangular(old_x1 - old_x0, old_y1));
      set_column(mold, 2, equirectangular(old_x2 - old_x0, old_y2));
  
      set_column(mnew, 0, equirectangular(0, new_y0));
      set_column(mnew, 1, equirectangular(new_x1 - new_x0, new_y1));
      set_column(mnew, 2, equirectangular(new_x2 - new_x0, new_y2));
        
      T = mul(T, mul(mold, inverse(mnew)));
      fixmatrix(T);
      
      si.matrixlist.push_back(T);
      si.zero_shift = new_x0;
      }
    }
  }

cpoint band_to_disk(int px, int py, sideinfo& si, int& tsiid) {

  cpoint c = pts[py][px].x;
  
  hyperpoint p;
  
  if(si.need_btd) {
  
    auto csi = &si;
    
    auto [x,y] = unband(c, *csi, 0);
    
    parent_changed:

    for(int subid: csi->childsides) {
      auto& nsi = sides[subid];
      auto& epts = *nsi.submap;
    
      auto [nx, ny] = unband(epts[py][px].x, nsi, 0);

      if(nx > nsi.zero_shift + 2) {
        x = nx; y = ny;
        csi = &nsi;
        goto parent_changed;
        }
      }
    
    p = {0, sinh(y), cosh(y)};
    p = mul(get_matrix_at(*csi, x), p);
    p = reperiod(p, si.period_matrices);
    tsiid = csi->id;
    }
  
  else {

    auto [x,y] = unband(c, si, -si.xcenter);
    
    if(si.period > 0) {
      ld d = si.period;
      while(x > d/2) x -= d;
      while(x < -d/2) x += d;
      }
     
    p = equirectangular(x, y);
    
    p = mul(spin(cspin), p);
    
    p = reperiod(p, si.period_matrices);
    }
  
  cpoint pt = hyper_to_disk(p);  
  return (cpoint{1, 1} + pt) * (si.img.s->h / 2);
  }
