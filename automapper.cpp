
ipoint get_last_point(pointmap& ptmap, ipoint start) {
  vector<ipoint> q;
  ptmap[start].type = 2; q.push_back(start);
  for(int i=0; i<isize(q); i++) {
    auto xy = q[i];
    for(auto k: dv) if(ptmap[k+xy].type == 1)
      ptmap[k+xy].type = 2, q.push_back(k + xy);
    }
  for(auto pt: q) ptmap[pt].type = 1;
  return q.back();
  }

void auto_mapin() {
  single_side(0);
  auto outpixel = get_heart(ipoint{0,0});
  
  ipoint start;

  for(int y=0; y<SY; y++) 
  for(int x=0; x<SX; x++) {
    auto xy = ipoint(x, y);
    auto& p = pts[xy];

    p.side = 0;
    if(get_heart(xy) != outpixel && x && y && x < SX-1 && y < SY-1)
      p.type = 1, start = xy;
    else
      p.type = 0;
    }
  
  ipoint axy = get_last_point(pts, start);
  ipoint bxy = get_last_point(pts, axy);
  printf("axy = %d,%d bxy = %d,%d\n", axy.x, axy.y, bxy.x, bxy.y);
  
  auto [axy1, ad] = boundary_point_near(pts, axy);
  auto [bxy1, bd] = boundary_point_near(pts, bxy);

  printf("axy1 = %d,%d bxy1 = %d,%d\n", axy1.x, axy1.y, bxy1.x, bxy1.y);
  
  split_boundary(pts, axy1, bxy1, bd^2);
  
  computemap(pts);
  
  measure(cside());
  
  int later = 0;
  
  vector<ipoint> q;
  for(int y=0; y<SY; y++) 
  for(int x=0; x<SX; x++) {
    auto xy = ipoint(x, y);
    auto& p = pts[xy];
    if(p.type != 1 || p.side != current_side) continue;
    if(p.x[1] > 1e-3 && p.x[1] < 1-1e-3) p.type = 2, q.push_back(xy);
    else later++;
    }
  
  printf("later = %d in = %d\n", later, isize(q));
  
  int nextd = isize(q);
  int d = 0;
  for(int i=0; i<isize(q); i++) {
    if(i == nextd) d++, nextd = isize(q);
    auto xy = q[i];
    for(auto k: dv) if(pts[k+xy].type == 1)
      pts[k+xy].type = 2, q.push_back(k + xy);
    }

  printf("d=%d until %d\n", d, nextd);
  
  ipoint ending = q.back();
  for(auto pt: q) pts[pt].type = 1;
  
  if(d >= 5) {
    auto& side = new_side(0);
    side.parentid = current_side;
    side.rootid = cside().rootid;
    cside().childsides.push_back(side.id);
    side.submap = new pointmap;
    // side.join = ?
    auto &epts = *side.submap;
    epts.resize2(SX, SY);
    
    ld error = 1e9;
    
    for(int y=0; y<SY; y++)
    for(int x=0; x<SX; x++) {
      auto xy = ipoint(x, y);
      auto& p = epts[xy];
      auto& pold = pts[xy];
      p.side = side.id;
      p.type = (pold.type == 1 && abs(pold.x[0] - pts[ending].x[0]) < cside().cscale[0]) ? 1 : 0;
      
      ld err = hypot(pold.x[0] - pts[ending].x[0], pold.x[1] - .5);
      if(err < error) error = err, side.join = xy;
      }
    
    printf("join = %d,%d ending = %d,%d\n", side.join.x, side.join.y, ending.x, ending.y);

    auto [axy2, ad2] = boundary_point_near(epts, side.join);
    auto [bxy2, bd2] = boundary_point_near(epts, ending);

    printf("axy = %d,%d bxy = %d,%d\n", axy2.x, axy2.y, bxy2.x, bxy2.y);
  
    split_boundary(epts, axy2, bxy2, bd2^2);
    computemap(epts);
    }
  }

