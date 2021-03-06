#include <vector>
#include <algorithm>

#include "steiner/graph.hpp"
#include "steiner/utils/utils.hpp"
#include "steiner/utils/point.hpp"
#include "steiner/utils/bottleneck_graph/bg_sleator_bias.hpp"

typedef Utils::Point Point;
typedef Utils::Edge  Edge;

/*
 * Implementation of BottleneckGraphSleatorBias constructor
 */
BottleneckGraphSleatorBias::BottleneckGraphSleatorBias(Graph &g)
  : BottleneckGraph(g), _vertices(g.n()), _pointsRef(g.getPointsRef()) {
  for(unsigned int i = 0; i < g.n(); i++) {
    this->_vertices[i].bparent  = NULL;
    this->_vertices[i].bleft    = NULL;
    this->_vertices[i].bright   = NULL;
    this->_vertices[i].bhead    = &this->_vertices[i];
    this->_vertices[i].btail    = &this->_vertices[i];
    this->_vertices[i].external = true;
    this->_vertices[i].reversed = false;
    this->_vertices[i].height   = 1;
    this->_vertices[i].idx      = i;
    this->_vertices[i].cost     = 0;
    this->_vertices[i].maxcost  = 0;
    this->_vertices[i].size     = 1;
    this->_vertices[i].w        = 1;
    this->_vertices[i].rank     = 0;
  }
  Utils::MSTKruskalMod(g);
  // Traverse tree
  std::vector<Edge> &edges = g.getEdges();
  for(unsigned int i = 0; i < edges.size(); i++) {
    Edge e = edges[i];
    this->evert(&this->_vertices[e.i0]);
    this->link(&this->_vertices[e.i0], &this->_vertices[e.i1], e.length);
  }
}

/*
 * Implementation of BottleneckGraphSleatorBias destructor
 */
BottleneckGraphSleatorBias::~BottleneckGraphSleatorBias() {
  // Assume all connected
  for(unsigned int i = 0; i < this->_vertices.size(); i++)
    if(this->_vertices[i].bparent)
      this->_cleanUp(this->path(&this->_vertices[i]));
}

void BottleneckGraphSleatorBias::_cleanUp(Vertex *v) {
  if(v->external)
    v->bparent = NULL;
  else {
    this->_cleanUp(v->bleft);
    this->_cleanUp(v->bright);
    delete v;
  }
}

/**
 * Implementation of BottleneckGraphSleatorBias::distance(...)
 */
double BottleneckGraphSleatorBias::distance(const unsigned int i, const unsigned int j) {
  if(i==j)
    return 0.0;
  this->evert(&this->_vertices[i]);
  Vertex *v = this->maxcost(&this->_vertices[j]);
  return v ? this->cost(v) : -1.0;
}

/**
 * Implementation of BottleneckGraphSleatorBias::contract(...)
 */
void BottleneckGraphSleatorBias::contract(const std::vector<unsigned int> &points) {
  if(points.size() < 2)
    return;
  
  // Remove the bottleneck edges
  for(unsigned int i=0; i<points.size(); i++) {
    this->evert(&this->_vertices[points[i]]);
    for(unsigned int j=i+1; j<points.size(); j++) {
      Path *p = this->expose(&this->_vertices[points[j]]);
      if(p == this->path(&this->_vertices[points[i]])) {
	// Connected
	Vertex *v = this->maxcost(&this->_vertices[points[j]]);
	if(v) // Should never be null here
	  this->cut(v);
      }
    }
  }

  this->evert(&this->_vertices[points[0]]);
  for(unsigned int i=1; i<points.size(); i++)
    this->link(&this->_vertices[points[0]], &this->_vertices[points[i]], 0.0);
}

// Static tree operations
BottleneckGraphSleatorBias::Vertex *BottleneckGraphSleatorBias::parent(Vertex *v) {
  return v == this->tail(this->path(v)) ? v->dparent : this->after(v);
}
BottleneckGraphSleatorBias::Vertex *BottleneckGraphSleatorBias::root(Vertex *v) {
  return this->tail(this->expose(v));
}
double BottleneckGraphSleatorBias::cost(Vertex *v) {
  return v == this->tail(this->path(v)) ? v->dcost : this->pcost(v);
}
BottleneckGraphSleatorBias::Vertex *BottleneckGraphSleatorBias::maxcost(Vertex *v) {
  return this->pmaxcost(this->expose(v));
}

// Dynamic tree operations
void BottleneckGraphSleatorBias::link(Vertex *v, Vertex *w, double x) {
  this->concatenate(this->path(v), this->expose(w), x);
}
double BottleneckGraphSleatorBias::cut(Vertex *v) {
  this->expose(v);
  SplitResult s;
  this->split(v, s);
  v->dparent = NULL;
  return s.y;
}
void BottleneckGraphSleatorBias::evert(Vertex *v) {
  this->reverse(this->expose(v));
  v->dparent = NULL;
}

// Path operations
BottleneckGraphSleatorBias::Path *BottleneckGraphSleatorBias::path(Vertex *v) {
  assert(v);
  while(v->bparent)
    v = v->bparent;
  return v;
}
BottleneckGraphSleatorBias::Vertex *BottleneckGraphSleatorBias::head(Path *p) {
  assert(p);
  return p->reversed ? p->btail : p->bhead;
}
BottleneckGraphSleatorBias::Vertex *BottleneckGraphSleatorBias::tail(Path *p) {
  assert(p);
  return p->reversed ? p->bhead : p->btail;
}
BottleneckGraphSleatorBias::Vertex *BottleneckGraphSleatorBias::before(Vertex *v) {
  RRS c;
  c.reversed = false;
  c.res      = NULL;
  this->before_rec(v, c);
  Vertex *w = c.res;
  if(!w)
    return NULL;
  Vertex *u = w->bparent->bleft == w ? w->bparent->bright : w->bparent->bleft;
  bool rev = (c.resrev != u->reversed); 
  if(u->external)
    return u;
  else
    return rev ? u->bhead : u->btail;
}
void BottleneckGraphSleatorBias::before_rec(Vertex *v, RRS &c) {
  Vertex *par = v->bparent;
  if(!par)
    // Root
    return;
  this->before_rec(par, c);
  c.reversed = (c.reversed != par->reversed);
  if(c.reversed) {
    if(v == par->bleft) {
      c.res = v;
      c.resrev = c.reversed;
    }
  }
  else if(v == par->bright) {
    c.res = v;
    c.resrev = c.reversed;
  }
}
BottleneckGraphSleatorBias::Vertex *BottleneckGraphSleatorBias::after(Vertex *v) {
  RRS c;
  c.reversed = false;
  c.res = NULL;
  this->after_rec(v, c);
  Vertex *w = c.res;
  if(!w)
    return NULL;
  Vertex *u = w->bparent->bleft == w ? w->bparent->bright : w->bparent->bleft;
  bool rev = (c.resrev != u->reversed); 
  if(u->external)
    return u;
  else
    return rev ? u->btail : u->bhead;
}
void BottleneckGraphSleatorBias::after_rec(Vertex *v, RRS &c) {
  Vertex *par = v->bparent;
  if(!par)
    // Root
    return;
  this->after_rec(par, c);
  c.reversed = (c.reversed != par->reversed);
  if(c.reversed) {
    if(v == par->bright) {
      c.res = v;
      c.resrev = c.reversed;
    }
  }
  else if(v == par->bleft) {
    c.res = v;
    c.resrev = c.reversed;
  }
}
double BottleneckGraphSleatorBias::pcost(Vertex *v) {
  RRS c;
  c.res = NULL;
  c.reversed = false;
  this->after_rec(v, c);
  Vertex *w = c.res->bparent;
  return w->cost;
}
BottleneckGraphSleatorBias::Vertex *BottleneckGraphSleatorBias::pmaxcost(Path *p) {
  bool rev = false;
  Vertex *u = p;
  Vertex *r;
  Vertex *l;
  while(true) {
    if(u->external)
      break;
    rev = (rev != u->reversed);
    if(rev) { r = u->bleft;  l = u->bright; }
    else    { r = u->bright; l = u->bleft;  }
    double diff = u->maxcost-u->cost;
    diff = diff < 0.0 ? -diff : diff;
    if(diff <= 0.0000001)
      // Edge has been found
      break;
    else
      u = (r->maxcost > l->maxcost && !r->external ? r : l);
  }
  if(u->external)
    return NULL;
  // Return rightmost external descendant of l
  if(l->external) return l;
  else if(rev != l->reversed) return l->bhead;
  else return l->btail;
}
void BottleneckGraphSleatorBias::reverse(Path *p) {
  p->reversed = !p->reversed;
}
BottleneckGraphSleatorBias::Path *BottleneckGraphSleatorBias::concatenate(Path *p, Path *q, double x) {
  return this->construct(p, q, x);
}
void BottleneckGraphSleatorBias::treepath(Vertex *v, Vertex **r, bool &reversed, bool before) {
  if(!v) {
    r = NULL;
    return;
  }
  Vertex *par = v->bparent;
  this->treepath(par, r, reversed, before);
  if(par) {
    if(reversed) {
      if((before && v == par->bleft) || (!before && v == par->bright)) {
	*r = par;
      }
    }
    else if((!before && v == par->bleft) || (before && v == par->bright)) {
      *r = par;
    }
  }
  reversed = (reversed != v->reversed);
}
void BottleneckGraphSleatorBias::tsplit(Vertex *v, bool right, DestructResult &dr) {
  Vertex *par = v->bparent;
  bool pright = (par && par->bright == v);
  if(!dr.left)
    this->destruct(v, dr);
  else {
    if(right) {
      v->bleft->bparent = NULL;
      dr.left = this->construct(v->bleft, dr.left, v->cost);
    }
    else {
      v->bright->bparent = NULL;
      dr.right = this->construct(dr.right, v->bright, v->cost);
    }
    
    if(v->reversed) {
      Vertex *tmp = dr.left;
      dr.left     = dr.right;
      dr.right    = tmp;
      
      dr.left->reversed  = !dr.left->reversed;
      dr.right->reversed = !dr.right->reversed;
    }
    delete v;
  }
  if(par)
    this->tsplit(par, pright, dr);
}
void BottleneckGraphSleatorBias::split(Vertex *v, SplitResult &res) {
  DestructResult dr1, dr2;
  dr1.left = NULL;
  dr1.right = NULL;
  dr2.left = NULL;
  dr2.right = NULL;
  
  Vertex *r = NULL;
  bool rev = false;
  this->treepath(v, &r, rev, true);
  if(r)
    this->tsplit(r, false, dr1);
  
  r = NULL;
  rev = false;
  this->treepath(v, &r, rev, false);
  if(r)
    this->tsplit(r, false, dr2);
  
  res.q = dr1.left;
  res.x = dr1.x;
  res.r = dr2.right;
  res.y = dr2.x;
}
  
// Splice and expose
BottleneckGraphSleatorBias::Path *BottleneckGraphSleatorBias::splice(Path *p) {
  Vertex *v = this->tail(p)->dparent;
  this->tail(p)->dparent = NULL;
  assert(v);
  SplitResult s;
  this->split(v, s);
  v->w -= p->w;
  if(s.q) {
    this->tail(s.q)->dparent = v;
    this->tail(s.q)->dcost   = s.x;
    v->w += s.q->w;
  }
  p = this->concatenate(p, this->path(v), this->tail(p)->dcost);
  p = s.r ? this->concatenate(p, s.r, s.y) : p;
  return p;
}
BottleneckGraphSleatorBias::Path *BottleneckGraphSleatorBias::expose(Vertex *v) {
  Path *p;
  SplitResult s;
  this->split(v, s);
  if(s.q) {
    this->tail(s.q)->dparent = v;
    this->tail(s.q)->dcost   = s.x;
    v->w += s.q->w;
  }
  p = s.r ? this->concatenate(this->path(v), s.r, s.y) : this->path(v);
  while(this->tail(p)->dparent) {p = this->splice(p);}
  return p;
}
  
// AVL tree operations
BottleneckGraphSleatorBias::Vertex *BottleneckGraphSleatorBias::construct(Vertex *v, Vertex *w, double x) {
  assert(v && w);
  this->unreverse(v);
  this->unreverse(w);
  if(v->rank == w->rank || (v->rank > w->rank && v->external) || (v->rank < w->rank && w->external)) {
    // Case 1
    Vertex *r = new Vertex;
    r->reversed = false;
    r->external = false;
    r->bparent  = NULL;
    r->bleft    = v;
    r->bright   = w;
    v->bparent  = r;
    w->bparent  = r;
    r->bhead    = this->head(r->bleft);
    r->btail    = this->tail(r->bright);
    r->cost     = x;
    r->maxcost  = v->maxcost > w->maxcost ? v->maxcost : w->maxcost;
    r->maxcost  = r->maxcost > x ? r->maxcost : x;
    r->idx      = r->bhead->idx*10+r->btail->idx;
    r->dparent  = NULL;
    r->dcost    = 0.0;
    r->height   = (v->height > w->height ? v->height : w->height) + 1;
    r->size     = v->size + w->size + 1;
    r->rank     = (v->rank > w->rank ? v->rank : w->rank) + 1;
    r->w        = v->w + w->w;
    
    return r;
  }
  else if(v->rank > w->rank) {
    // Case 2
    this->tiltleft(v);
    Vertex *z = v->bright;
    v->bright = this->construct(z, w, x);
    v->bright->bparent = v;
    v->maxcost = v->bright->maxcost > v->maxcost ? v->bright->maxcost : v->maxcost;
    v->btail   = this->tail(v->bright);
    v->height  = (v->bleft->height > v->bright->height ? v->bleft->height : v->bright->height) + 1;
    v->size    = v->bleft->size + v->bright->size + 1;
    return v;
  }
  else { // (v->rank < w->rank)
    // Case 3
    this->tiltright(w);
    Vertex *z = w->bleft;
    w->bleft = this->construct(v, z, x);
    w->bleft->bparent = w;
    w->maxcost = w->bleft->maxcost > w->maxcost ? w->bleft->maxcost : w->maxcost;
    w->bhead   = this->head(w->bleft);
    w->height = (w->bleft->height > w->bright->height ? w->bleft->height : w->bright->height) + 1;
    w->size   = w->bleft->size + w->bright->size + 1;
    return w;
  }
}
void BottleneckGraphSleatorBias::destruct(Vertex *u, DestructResult &c) {
  this->unreverse(u);
  c.left = u->bleft;
  c.left->bparent = NULL;
  c.right = u->bright;
  c.right->bparent = NULL;
  c.x = u->cost;
  delete u;
}

void BottleneckGraphSleatorBias::tiltleft(Vertex *v) {
  assert(v);
  assert(v->bleft && v->bright);
  this->unreverse(v->bright);
  if(v->bright->rank == v->rank) {
    if(v->bleft->rank == v->rank)
      v->rank++;
    else
      this->rotateleft(v);
  }
}

void BottleneckGraphSleatorBias::tiltright(Vertex *v) {
  assert(v);
  assert(v->bleft && v->bright);
  this->unreverse(v->bleft);
  if(v->bleft->rank == v->rank) {
    if(v->bright->rank == v->rank)
      v->rank++;
    else
      this->rotateright(v);
  }
}

void BottleneckGraphSleatorBias::rotateleft(Vertex *v) {
  assert(v);
  Vertex *rc = v->bright;
  assert(rc);
  Vertex *p = v->bleft;
  Vertex *q = rc->bleft;
  Vertex *r = rc->bright;
  assert(p && q && r);
  
  // Update v
  v->bleft   = rc;
  v->bright  = r;
  // Update p
  if(p) p->bparent = rc;
  // Update r
  if(r) r->bparent = v;
  // Update rc
  rc->bleft  = p;
  rc->bright = q;

  // Update costs
  double vc = v->cost;
  v->cost   = rc->cost;
  rc->cost  = vc;
  
  // Update heights
  rc->height = (p->height > q->height ? p->height : q->height)+1;
  v->height  = (rc->height > r->height ? rc->height : r->height)+1;

  // Update maxcost
  rc->maxcost = (p->maxcost > q->maxcost ? p->maxcost : q->maxcost);
  rc->maxcost = rc->cost > rc->maxcost ? rc->cost : rc->maxcost; 
  
  v->maxcost = r->maxcost > rc->maxcost ? r->maxcost : rc->maxcost;
  v->maxcost = v->cost > v->maxcost ? v->cost : v->maxcost;

  // Update size and weight
  rc->size = p->size + q->size + 1;
  rc->w    = p->w + q->w;
  v->size  = rc->size + r->size + 1;
  v->w     = rc->w + r->w;
  
  // Update bhead and btail
  if(!rc->external) {
    // rc->reversed == false
    assert(rc->bleft && rc->bright);
    rc->btail = this->tail(rc->bright);
    rc->bhead = this->head(rc->bleft);
  }
  if(!v->external) {
    // v->reversed == false
    assert(v->bleft && v->bright);
    v->btail = this->tail(v->bright);
    v->bhead = this->head(v->bleft);
  }
}
void BottleneckGraphSleatorBias::rotateright(Vertex *v) {
  assert(v);
  Vertex *lc = v->bleft;
  assert(lc);
  Vertex *p = lc->bleft;
  Vertex *q = lc->bright;
  Vertex *r = v->bright;
  assert(p && q && r);
  
  // Update v
  v->bleft   = p;
  v->bright  = lc;
  // Update p
  if(p) p->bparent = v;
  // Update q
  if(r) r->bparent = lc;
  // Update rc
  lc->bleft  = q;
  lc->bright = r;

  // Update costs
  double vc = v->cost;
  v->cost   = lc->cost;
  lc->cost  = vc;
  
  // Update heights
  lc->height = (q->height > r->height ? q->height : r->height)+1;
  v->height  = (lc->height > p->height ? lc->height : p->height)+1;
  
  // Update maxcost
  lc->maxcost = q->maxcost > r->maxcost ? q->maxcost : r->maxcost;
  lc->maxcost = lc->cost > lc->maxcost ? lc->cost : lc->maxcost; 
  
  v->maxcost = p->maxcost > lc->maxcost ? p->maxcost : lc->maxcost;
  v->maxcost = v->cost > v->maxcost ? v->cost : v->maxcost;
  
  // Update size and weight
  lc->size = q->size + r->size + 1;
  lc->w    = q->w + r->w;
  v->size  = lc->size + p->size + 1;
  v->w     = lc->w + p->w;

  // Update bhead and btail
  if(!lc->external) {
    // lc->reversed == false
    assert(lc->bleft && lc->bright);
    lc->btail = this->tail(lc->bright);
    lc->bhead = this->head(lc->bleft);
  }
  if(!v->external) {
    // v->reversed == false
    assert(v->bleft && v->bright);
    v->btail = this->tail(v->bright);
    v->bhead = this->head(v->bleft);
  }
}
void BottleneckGraphSleatorBias::unreverse(Vertex *v) {
  if(v->reversed && !v->external) {
    v->reversed = false;
    Vertex *tmp = v->bleft;
    v->bleft    = v->bright;
    v->bright   = tmp;
    tmp = v->btail;
    v->btail = v->bhead;
    v->bhead = tmp;

    v->bleft->reversed  = !v->bleft->reversed;
    v->bright->reversed = !v->bright->reversed;
  }
}
