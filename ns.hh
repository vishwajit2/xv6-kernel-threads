#pragma once

#include "gc.hh"

// name spaces
// XXX maybe use open hash table, no chain, better cache locality

#if SPINLOCK_DEBUG
#define NHASH 10
#else
#define NHASH 100
#endif

class scoped_gc_epoch {
 public:
  scoped_gc_epoch() { gc_begin_epoch(); }
  ~scoped_gc_epoch() { gc_end_epoch(); }
};

template<class K, class V>
class xelem : public rcu_freed {
 public:
  V val;
  int next_lock;
  xelem<K, V> * volatile next;
  K key;

  xelem(const K &k, const V &v) : rcu_freed("xelem"), val(v), next_lock(0), next(0), key(k) {}
  virtual ~xelem() {}
};

template<class K, class V>
struct xbucket {
  xelem<K, V> * volatile chain;
} __attribute__((aligned (CACHELINE)));

template<class K, class V, u64 (*HF)(const K&)>
class xns : public rcu_freed {
 private:
  bool allowdup;
  u64 nextkey;
  xbucket<K, V> table[NHASH];

 public:
  xns(bool dup) : rcu_freed("xns") {
    allowdup = dup;
    nextkey = 1;
    for (int i = 0; i < NHASH; i++)
      table[i].chain = 0;
  }

  ~xns() {
    for (int i = 0; i < NHASH; i++)
      if (table[i].chain)
        panic("~xns: not empty");
  }

  u64 allockey() {
    return __sync_fetch_and_add(&nextkey, 1);
  }

  u64 h(const K &key) {
    return HF(key) % NHASH;
  }

  int insert(const K &key, const V &val) {
    auto e = new xelem<K, V>(key, val);
    if (!e)
      return -1;

    u64 i = h(key);
    scoped_gc_epoch gc;

    for (;;) {
      auto root = table[i].chain;
      if (!allowdup) {
        for (auto x = root; x; x = x->next) {
          if (x->key == key) {
            gc_delayed(e);
            return -1;
          }
        }
      }

      e->next = root;
      if (__sync_bool_compare_and_swap(&table[i].chain, root, e))
        return 0;
    }
  }

  V lookup(const K &key) {
    u64 i = h(key);

    scoped_gc_epoch gc;
    auto e = table[i].chain;

    while (e) {
      if (e->key == key)
        return e->val;
      e = e->next;
    }

    return 0;
  }

  bool remove(const K &key, const V *vp = 0) {
    u64 i = h(key);
    scoped_gc_epoch gc;

    for (;;) {
      int fakelock = 0;
      int *pelock = &fakelock;
      auto pe = &table[i].chain;

      for (;;) {
        auto e = *pe;
        if (!e)
          return false;

        if (e->key == key && (!vp || e->val == *vp)) {
          if (!__sync_bool_compare_and_swap(&e->next_lock, 0, 1))
            break;
          if (!__sync_bool_compare_and_swap(pelock, 0, 1)) {
            e->next_lock = 0;
            break;
          }
          if (!__sync_bool_compare_and_swap(pe, e, e->next)) {
            *pelock = 0;
            e->next_lock = 0;
            break;
          }

          *pelock = 0;
          gc_delayed(e);
          return true;
        }

        pe = &e->next;
        pelock = &e->next_lock;
      }
    }
  }

  template<class CB>
  void enumerate(CB cb) {
    scoped_gc_epoch gc;
    for (int i = 0; i < NHASH; i++) {
      auto e = table[i].chain;
      while (e) {
        if (cb(e->key, e->val))
          return;
        e = e->next;
      }
    }
  }

  template<class CB>
  void enumerate_key(const K &key, CB cb) {
    scoped_gc_epoch gc;
    u64 i = h(key);
    auto e = table[i].chain;
    while (e) {
      if (e->key == key && cb(e->key, e->val))
        return;
      e = e->next;
    }
  }

  class iterator {
  private:
    xns<K, V, HF> *ns_;
    xelem<K, V> *chain_;
    int ndx_;

  public:
    iterator(xns<K, V, HF> *ns) {
      if (ns_)
        gc_begin_epoch();
      ns_ = ns;
      ndx_ = 0;
      chain_ = ns->table[ndx_++].chain;
      for (; chain_ == 0 && ndx_ < NHASH; ndx_++)
        chain_ = ns_->table[ndx_].chain;
    }

    iterator() {
      ns_ = 0;
      ndx_ = NHASH;
      chain_ = 0;
    }

    ~iterator() {
      if (ns_)
        gc_end_epoch();
    }

    bool operator!=(const iterator &other) const {
      return other.chain_ != this->chain_;
    }

    iterator& operator ++() {
      for (chain_ = chain_->next; chain_ == 0 && ndx_ < NHASH; ndx_++)
        chain_ = ns_->table[ndx_].chain;
      return *this;
    }

    V& operator *() {
      return chain_->val;
    }
  };

  iterator begin() {
    return iterator(this);
  }

  iterator end() {
    return iterator();
  }
};

template<class K, class V, u64 (*HF)(const K&)>
static inline
typename xns<K, V, HF>::iterator
begin(xns<K, V, HF> *&ns)
{
  return ns->begin();
}

template<class K, class V, u64 (*HF)(const K&)>
static inline
typename xns<K, V, HF>::iterator
end(xns<K, V, HF> *&ns)
{
  return ns->end();
}