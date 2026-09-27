#include "scope_resources.hpp"
namespace rev_hud {
namespace {
bool read(Reader r,u32 a,u32& v) noexcept {
    return a && a<=Invalid-3 && r.word && r.word(r.context,a,&v);
}
bool tree_has(const GuiTree& t,u32 a) noexcept {
    if(a==t.unit || a==t.root || a==t.table || a==t.resource)return true;
    for(u32 i=0;i<t.count;++i)if(a==t.nodes[i])return true;
    return false;
}
bool once(Reader r,u32 unit,ScopeResources& s) noexcept {
    if(!unit || unit>Invalid-0x2AF || !capture_tree(r,unit,WidgetKind::Scope,&s.tree) ||
       s.tree.count<3 || !read(r,unit+0x224,s.count) || !s.count ||
       s.count>ScopeResources::MaxAnimations || !read(r,unit+0x230,s.table) ||
       !s.table || s.table>Invalid-s.count*4 || tree_has(s.tree,s.table))return false;
    for(u32 i=0;i<3;++i){u32 v=0;
        if(!read(r,unit+0x2A0+i*4,v) || v!=s.tree.nodes[i])return false;
    }
    for(u32 i=0;i<s.count;++i){
        auto& a=s.animations[i];
        if(!read(r,s.table+i*4,a) || !a || a==s.table || tree_has(s.tree,a) ||
           !read(r,a,s.vtables[i]) || !s.vtables[i])return false;
        for(u32 j=0;j<i;++j)if(a==s.animations[j])return false;
    }
    u32 cached=0;return read(r,unit+0x2AC,cached) && cached==s.animations[0];
}
}
bool same_scope(const ScopeResources& a,const ScopeResources& b) noexcept {
    const auto& x=a.tree;const auto& y=b.tree;
    if(!x.unit || x.unit!=y.unit || x.resource!=y.resource || x.root!=y.root ||
       x.table!=y.table || !x.count || x.count!=y.count || x.count>GuiTree::MaxNodes ||
       a.table!=b.table || !a.count || a.count!=b.count || a.count>ScopeResources::MaxAnimations)return false;
    for(u32 i=0;i<x.count;++i)if(x.nodes[i]!=y.nodes[i])return false;
    for(u32 i=0;i<a.count;++i)if(a.animations[i]!=b.animations[i] || a.vtables[i]!=b.vtables[i])return false;
    return true;
}
bool capture_scope(Reader r,u32 unit,ScopeResources* out) noexcept {
    if(!out)return false;
    ScopeResources a{},b{};
    if(!once(r,unit,a) || !once(r,unit,b) || !same_scope(a,b))return false;
    *out=a;return true;
}
bool disjoint_scopes(const ScopeResources& a,const ScopeResources& b) noexcept {
    if(!same_scope(a,a) || !same_scope(b,b) || !disjoint_trees(a.tree,b.tree))return false;
    for(u32 i=0;i<=a.count;++i){const u32 x=i?a.animations[i-1]:a.table;
        if(tree_has(b.tree,x))return false;
        for(u32 j=0;j<=b.count;++j)if(x==(j?b.animations[j-1]:b.table))return false;
    }
    for(u32 i=0;i<=b.count;++i)if(tree_has(a.tree,i?b.animations[i-1]:b.table))return false;
    return true;
}
}
