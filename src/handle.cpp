#include "handle.hpp"
#include "snarls.hpp"

/** \file handle.cpp
 * Implement handle graph utility methods.
 */

namespace vg {

using namespace std;

handle_t HandleGraph::get_handle(const Visit& visit) const {
    return get_handle(visit.node_id(), visit.backward());
}

Visit HandleGraph::to_visit(const handle_t& handle) const {
    return vg::to_visit(this->get_id(handle), this->get_is_reverse(handle));
}

handle_t HandleGraph::forward(const handle_t& handle) const {
    return this->get_is_reverse(handle) ? this->flip(handle) : handle;
}

pair<handle_t, handle_t> HandleGraph::edge_handle(const handle_t& left, const handle_t& right) const {
    // The degeneracy is between any pair and a pair of the same nodes but reversed in order and orientation.
    // We compare those two pairs and construct the smaller one.
    auto flipped_right = this->flip(right);
    
    if (as_integer(left) > as_integer(flipped_right)) {
        // The other orientation would be smaller.
        return make_pair(flipped_right, this->flip(left));
    } else if(as_integer(left) == as_integer(flipped_right)) {
        // Our left and the flipped pair's left would be equal.
        auto flipped_left = this->flip(left);
        if (as_integer(right) > as_integer(flipped_left)) {
            // And our right is too big, so flip.
            return make_pair(flipped_right, flipped_left);
        } else {
            // No difference or we're smaller.
            return make_pair(left, right);
        }
    } else {
        // We're smaller
        return make_pair(left, right);
    }
}

}


