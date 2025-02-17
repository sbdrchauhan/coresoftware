// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef GLOBALVERTEX_GLOBALVERTEXV1_H
#define GLOBALVERTEX_GLOBALVERTEXV1_H

#include "GlobalVertex.h"

#include <cstddef>  // for size_t
#include <iostream>
#include <limits>
#include <map>
#include <utility>  // for pair, make_pair

class PHObject;

class GlobalVertexv1 : public GlobalVertex
{
 public:
  GlobalVertexv1(const GlobalVertex::VTXTYPE id = GlobalVertex::UNDEFINED);
  ~GlobalVertexv1() override = default;

  // PHObject virtual overloads

  void identify(std::ostream& os = std::cout) const override;
  void Reset() override { *this = GlobalVertexv1(); }
  int isValid() const override;
  PHObject* CloneMe() const override { return new GlobalVertexv1(*this); }

  // vertex info

  unsigned int get_id() const override { return _id; }
  void set_id(unsigned int id) override { _id = id; }

  float get_t() const override { return _t; }
  void set_t(float t) override { _t = t; }

  float get_t_err() const override { return _t_err; }
  void set_t_err(float t_err) override { _t_err = t_err; }

  float get_x() const override { return _pos[0]; }
  void set_x(float x) override { _pos[0] = x; }

  float get_y() const override { return _pos[1]; }
  void set_y(float y) override { _pos[1] = y; }

  float get_z() const override { return _pos[2]; }
  void set_z(float z) override { _pos[2] = z; }

  float get_chisq() const override { return _chisq; }
  void set_chisq(float chisq) override { _chisq = chisq; }

  unsigned int get_ndof() const override { return _ndof; }
  void set_ndof(unsigned int ndof) override { _ndof = ndof; }

  float get_position(unsigned int coor) const override { return _pos[coor]; }
  void set_position(unsigned int coor, float xi) override { _pos[coor] = xi; }

  float get_error(unsigned int i, unsigned int j) const override;        //< get vertex error covar
  void set_error(unsigned int i, unsigned int j, float value) override;  //< set vertex error covar

  //
  // associated vertex ids methods
  //

  bool empty_vtxids() const override { return _vtx_ids.empty(); }
  size_t size_vtxids() const override { return _vtx_ids.size(); }
  size_t count_vtxids(GlobalVertex::VTXTYPE type) const override { return _vtx_ids.count(type); }

  void clear_vtxids() override { _vtx_ids.clear(); }
  void insert_vtxids(GlobalVertex::VTXTYPE type, unsigned int vtxid) override { _vtx_ids.insert(std::make_pair(type, vtxid)); }
  size_t erase_vtxids(GlobalVertex::VTXTYPE type) override { return _vtx_ids.erase(type); }
  void erase_vtxids(GlobalVertex::VtxIter iter) override { _vtx_ids.erase(iter); }
  void erase_vtxids(GlobalVertex::VtxIter first, GlobalVertex::VtxIter last) override { _vtx_ids.erase(first, last); }

  GlobalVertex::ConstVtxIter begin_vtxids() const override { return _vtx_ids.begin(); }
  GlobalVertex::ConstVtxIter find_vtxids(GlobalVertex::VTXTYPE type) const override { return _vtx_ids.find(type); }
  GlobalVertex::ConstVtxIter end_vtxids() const override { return _vtx_ids.end(); }

  GlobalVertex::VtxIter begin_vtxids() override { return _vtx_ids.begin(); }
  GlobalVertex::VtxIter find_vtxids(GlobalVertex::VTXTYPE type) override { return _vtx_ids.find(type); }
  GlobalVertex::VtxIter end_vtxids() override { return _vtx_ids.end(); }

 private:
  unsigned int covar_index(unsigned int i, unsigned int j) const;

  unsigned int _id = std::numeric_limits<unsigned int>::max();  //< unique identifier within container
  float _t = std::numeric_limits<float>::quiet_NaN();           //< collision time
  float _t_err = std::numeric_limits<float>::quiet_NaN();       //< collision time uncertainty
  float _pos[3] = {};                                           //< collision position x,y,z
  float _chisq = std::numeric_limits<float>::quiet_NaN();       //< vertex fit chisq
  unsigned int _ndof = std::numeric_limits<unsigned int>::max();
  float _err[6] = {};                                      //< error covariance matrix (+/- cm^2)
  std::map<GlobalVertex::VTXTYPE, unsigned int> _vtx_ids;  //< list of vtx ids

  ClassDefOverride(GlobalVertexv1, 1);
};

#endif
