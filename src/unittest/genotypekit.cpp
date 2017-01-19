/**
 * unittest/genotypekit.cpp: test cases for genotypekit modular genotyper pieces
 */

#include "catch.hpp"
#include "genotypekit.hpp"

namespace Catch {

    // Make it so we can see node pointers
    std::string toString(vg::Node* const& value) {
        if(value != nullptr) {
            return "&<Node " + std::to_string(value->id()) + ">";
        } else {
            return "&<null node>";
        }
    }
    
    // Make it so we can see edge pointers
    std::string toString(vg::Edge* const& value) {
        if(value != nullptr) {
            auto sides = vg::NodeSide::pair_from_edge(value);
            std::stringstream stream;
            stream << "&<Edge " << sides.first << " -> " << sides.second << ">";
            return stream.str();
        } else {
            return "&<null edge>";
        }
    }

    
    // And so that we can see sets of things
    template<typename Item> struct StringMaker<std::set<Item>> {
        static std::string convert(std::set<Item> const& value) {
            std::stringstream stream;
            stream << "{";
            
            
            for(auto i = value.begin(); i != value.end(); ++i) {
                if(i != value.begin()) {
                    // We need the separator
                    stream << ", ";
                }
                
                stream << toString(*i);
            }
            
            stream << "}";
            return stream.str();
        }
    };
    
}

namespace vg {
namespace unittest {

TEST_CASE("sites can be found with Cactus", "[genotype]") {
    
    // Build a toy graph
    const string graph_json = R"(
    
    {
        "node": [
            {"id": 1, "sequence": "G"},
            {"id": 2, "sequence": "A"},
            {"id": 3, "sequence": "T"},
            {"id": 4, "sequence": "GGG"},
            {"id": 5, "sequence": "T"},
            {"id": 6, "sequence": "A"},
            {"id": 7, "sequence": "C"},
            {"id": 8, "sequence": "A"},
            {"id": 9, "sequence": "A"}
        ],
        "edge": [
            {"from": 1, "to": 2},
            {"from": 1, "to": 6},
            {"from": 2, "to": 3},
            {"from": 2, "to": 4},
            {"from": 3, "to": 5},
            {"from": 4, "to": 5},
            {"from": 5, "to": 6},
            {"from": 6, "to": 7},
            {"from": 6, "to": 8},
            {"from": 7, "to": 9},
            {"from": 8, "to": 9}
            
        ],
        "path": [
            {"name": "hint", "mapping": [
                {"position": {"node_id": 1}},
                {"position": {"node_id": 6}},
                {"position": {"node_id": 8}},
                {"position": {"node_id": 9}}
            ]}
        ]
    }
    
    )";
    
    // Make an actual graph
    VG graph;
    Graph chunk;
    json2pb(chunk, graph_json.c_str(), graph_json.size());
    graph.merge(chunk);
    
    // Make a CactusUltrabubbleFinder
    SnarlFinder* finder = new CactusUltrabubbleFinder(graph, "hint");
    
    SECTION("CactusUltrabubbleFinder should find two top-level sites") {
        
        SnarlManager manager = finder->find_snarls();
        
        auto sites = manager.top_level_snarls();
        
        REQUIRE(sites.size() == 2);
        
        // Order them
        const Snarl* site_1 = sites[0]->start().node_id() > sites[1]->start().node_id() ? sites[1] : sites[0];
        const Snarl* site_2 = sites[0]->start().node_id() > sites[1]->start().node_id() ? sites[0] : sites[1];
        
        SECTION("the first site should be 1 fwd to 6 fwd") {
            REQUIRE(site_1->start().node_id() == 1);
            REQUIRE(site_1->start().backward() == false);
            REQUIRE(site_1->end().node_id() == 6);
            REQUIRE(site_1->end().backward() == false);
            
            SECTION("and should contain exactly nodes 1 through 6") {
                auto nodes = manager.deep_contents(site_1, graph, true).first;
                set<Node*> correct{graph.get_node(1), graph.get_node(2),
                        graph.get_node(3), graph.get_node(4),
                        graph.get_node(5), graph.get_node(6)};
                
                REQUIRE(nodes.size() == correct.size());
                for (Node* node : nodes) {
                    REQUIRE(correct.count(node));
                }
            }
            
            SECTION("and should contain exactly edges 1->6, 1->2, 2->3, 2->4, 3->5, 4->5, 5->6") {
                auto edges = manager.deep_contents(site_1, graph, true).second;
                set<Edge*> correct{
                    graph.get_edge(NodeSide(1, true), NodeSide(6)),
                    graph.get_edge(NodeSide(1, true), NodeSide(2)),
                    graph.get_edge(NodeSide(2, true), NodeSide(3)),
                    graph.get_edge(NodeSide(2, true), NodeSide(4)),
                    graph.get_edge(NodeSide(3, true), NodeSide(5)),
                    graph.get_edge(NodeSide(4, true), NodeSide(5)),
                    graph.get_edge(NodeSide(5, true), NodeSide(6))
                };
                
                REQUIRE(edges.size() == correct.size());
                for (Edge* edge : edges) {
                    REQUIRE(correct.count(edge));
                }
            }
            
            SECTION("and should contain one child") {
                REQUIRE(manager.children_of(site_1).size() == 1);
                
                auto& child = manager.children_of(site_1)[0];
                
                SECTION("that child should be 2 fwd to 5 fwd") {
                    REQUIRE(child->start().node_id() == 2);
                    REQUIRE(child->start().backward() == false);
                    REQUIRE(child->end().node_id() == 5);
                    REQUIRE(child->end().backward() == false);
                }
            }
            
        }
        
        SECTION("the second site should be 6 fwd to 9 fwd") {
            REQUIRE(site_2->start().node_id() == 6);
            REQUIRE(site_2->start().backward() == false);
            REQUIRE(site_2->end().node_id() == 9);
            REQUIRE(site_2->end().backward() == false);
            
            SECTION("and should contain no children") {
                REQUIRE(manager.children_of(site_2).size() == 0);
            }
        }
        
        
    }
    
    delete finder;

}

TEST_CASE("fixed priors can be assigned to genotypes", "[genotype]") {
    
    GenotypePriorCalculator* calculator = new FixedGenotypePriorCalculator();
    
    Genotype het;
    het.add_allele(0);
    het.add_allele(1);
    
    Genotype hom_alt;
    hom_alt.add_allele(1);
    hom_alt.add_allele(1);
    
    Genotype hom_ref;
    hom_ref.add_allele(0);
    hom_ref.add_allele(0);
    
    SECTION("homozygote priors should be equal") {
        REQUIRE(calculator->calculate_log_prior(hom_alt) == calculator->calculate_log_prior(hom_ref));
    }
    
    SECTION("homozygotes should be more likely than heterozygotes") {
        REQUIRE(calculator->calculate_log_prior(het) < calculator->calculate_log_prior(hom_ref));
        REQUIRE(calculator->calculate_log_prior(het) < calculator->calculate_log_prior(hom_alt));
    }
    
    SECTION("haploid genotypes should have nonzero prior") {
        Genotype haploid;
        haploid.add_allele(5);
        REQUIRE(calculator->calculate_log_prior(haploid) > prob_to_logprob(0));
    }
    
    SECTION("zero-ploid genotypes should have nonzero prior") {
        Genotype empty;
        REQUIRE(calculator->calculate_log_prior(empty) > prob_to_logprob(0));
    }
    
    SECTION("polyploid genotypes should have nonzero prior") {
        Genotype polyploid;
        for(int i = 0; i < 100; i++) {
            polyploid.add_allele(i);
        }
        REQUIRE(calculator->calculate_log_prior(polyploid) > prob_to_logprob(0));
    }
    
    delete calculator;
}

TEST_CASE("TrivialTraversalFinder can find traversals", "[genotype]") {
    // Build a toy graph
    const string graph_json = R"(
    
    {
        "node": [
            {"id": 1, "sequence": "G"},
            {"id": 2, "sequence": "A"},
            {"id": 3, "sequence": "T"},
            {"id": 4, "sequence": "GGG"},
            {"id": 5, "sequence": "T"},
            {"id": 6, "sequence": "A"},
            {"id": 7, "sequence": "C"},
            {"id": 8, "sequence": "A"},
            {"id": 9, "sequence": "A"}
        ],
        "edge": [
            {"from": 1, "to": 2},
            {"from": 1, "to": 6},
            {"from": 2, "to": 3},
            {"from": 2, "to": 4},
            {"from": 3, "to": 5},
            {"from": 4, "to": 5},
            {"from": 5, "to": 6},
            {"from": 6, "to": 7},
            {"from": 6, "to": 8},
            {"from": 7, "to": 9},
            {"from": 8, "to": 9}
            
        ],
        "path": [
            {"name": "hint", "mapping": [
                {"position": {"node_id": 1}},
                {"position": {"node_id": 6}},
                {"position": {"node_id": 8}},
                {"position": {"node_id": 9}}
            ]}
        ]
    }
    
    )";
    
    // Make an actual graph
    VG graph;
    Graph chunk;
    json2pb(chunk, graph_json.c_str(), graph_json.size());
    graph.merge(chunk);
    
    // Make a site
    Snarl site;
    site.mutable_start()->set_node_id(2);
    site.mutable_end()->set_node_id(5);
    site.set_type(ULTRABUBBLE);
    
    // Make the TraversalFinder
    TraversalFinder* finder = new TrivialTraversalFinder(graph);
    
    SECTION("at least one path can be found") {
        auto site_traversals = finder->find_traversals(site);
        
        REQUIRE(!site_traversals.empty());
        
        SECTION("the path must visit 1 node to span the site") {
            REQUIRE(site_traversals.front().visits_size() == 1);
            
            SECTION("the site must follow one of the two paths in the correct orientation") {
                bool followed_path_1 = site_traversals.front().visits(0).node_id() == 3 &&
                                       site_traversals.front().visits(0).backward() == false;
                
                bool followed_path_2 = site_traversals.front().visits(0).node_id() == 4 &&
                                       site_traversals.front().visits(0).backward() == false;
                
                REQUIRE(followed_path_1 != followed_path_2);
            }
        }
    }
    
    delete finder;
    

}

}
}
