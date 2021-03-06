/**
 * Test of the gdl_window
 */
#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h>

#include "libclap/clap.hpp"
#include "test/test.hpp"
#include "steiner/utils/delaunay.hpp"
#include "steiner/utils/utils.hpp"
#include "steiner/utils/point.hpp"
#include "steiner/utils/point_set_generator.hpp"
#include "steiner/graph.hpp"
#include "steiner/esmt.hpp"
#include "steiner/steiner_tree.hpp"
#include "steiner/heuristics/subgraph_heuristic.hpp"
#include "steiner/heuristics/steiner_finder.hpp"
#include "steiner/heuristics/smith.hpp"
#include "steiner/heuristics/concat.hpp"

#define MAX_DIM     12

#ifdef ESMT_TEST_MODE
#define PATTERN_BATCH    0
#define PATTERN_IN_FILE  1
#define PATTERN_GENERATE 2
#else
#define PATTERN_IN_FILE  0
#define PATTERN_GENERATE 1
#endif

typedef Utils::Point      Point;
typedef Utils::Edge       Edge;
typedef Utils::Generator  Generator;

int set_type_from_name(std::string &settype) {
  int t = -1;
  if(settype.compare(0, 20, "random") == 0)
    t = TEST_POINT_SET_RANDOM_D;
  else if(settype.compare(0, 20, "simplex") == 0)
    t = TEST_POINT_SET_SIMPLEX;
  else if(settype.compare(0, 20, "sausage") == 0)
    t = TEST_POINT_SET_SAUSAGE;
  else if(settype.compare(0, 20, "grid") == 0)
    t = TEST_POINT_SET_GRID;
  else if(settype.compare(0, 20, "delaunay") == 0)
    t = TEST_POINT_SET_DELAUNAY_SIMPLICES;
  return t;
}

int main(int argc, char *argv[]) {
  try {
    const std::string clap_config =
      "DESCRIPTION: \n"
      "file is a STP file and name is the name of a set in that file.\n"
      "type is a set type (random, grid, sausage), d is the dimension and n is the number of points to generate.\n"
      "OPTIONS: \n"
#ifdef ESMT_TEST_MODE
      "-os   --onlysgh                           'Test only subgraph heuristic. Use with -b option.' \n"
      "-ct   --counttest  br d:i n:i s:i         'Run the count test.' \n"
      "-tt   --testtopo   br                     'Run the topology test.' \n"
#endif
      "-v    --verbose                           'Print info during execution.' \n"
      "-npo  --nopostopt                         'Disable fine tuning.' \n"
      "-nsc  --nosubcon                          'Disable subgraph concatenation/sausages.' \n"
      "-rdc  --redocon                           'Enable concatenation redo.' \n"
      "-ubd  --usebdist      type:i              'Use bottleneck distances. type must be 1 (table computation), 2 (lazy computation) or 3 (dyn. trees).' \n"
      "-k    --facemaxsize   type:i              'Set the maximum face size to consider for concatenation. Only applicable when using -ubd.' \n"
      "-alg  --alg           name:s              'Set the subgraph heuristic to use. Must be NO, RNO or SP.' \n"
      "-s    --seed          s:i                 'Use seed s for generating random point sets.' \n"
      "-pt   --printtree                         'Print tree when done.' \n"
      "-val  --validate                          'Validate tree when done.' \n"
      "-st   --stats                             'Collect extra stats.' \n"
#ifdef ESMT_TEST_MODE
      "-nd   --nodelaunay                        'Do not include Delaunay in timing. Use with -b option.' \n"
      "-out  --outfile       path:s              'File to store results in. Use with -b option.' \n"
      "-lt   --looptime      t:i                 'Run tests in a loop for t seconds. Use with -b option.' \n"
      "-b    --batch                             'Run as a batch and time runs. Add tests with -g, -gn, -if and -ifa' \n"
      "-g    --generate      type:s d:i n:i      'Generate a point set of size n in d dimensions. type must be random, grid or sausage. Use with -b option.' \n"
      "-gn   --generateN     type:s d:i n:i m:i  'Generate m point sets of size n in d dimensions. set_type must be random, grid or sausage. Use with -b option.' \n"
      "-if   --infile        file:s name:s       'Read set with given name from file. Use with -b option.' \n"
      "-ifa  --infileall     file:s              'Read all sets from a file. Use with -b option.' \n"
#endif
      "PARAMETERS: \n"
#ifdef ESMT_TEST_MODE
      "\n"
#endif
      "file:s name:s \n"
      "type:s d:i n:i \n";
    
    CLAP c(clap_config, argc, argv);
  
#ifdef ESMT_TEST_MODE
    if(c.is_set("testtopo")) {
      std::cout << "Running topo test!" << std::endl;
      std::string name("Smith iterative");
      std::string resFile("/home/stephan/Desktop/result.dat");
      std::string path("../data/protein3D/W1.stp");
      std::string sname("W1");
      std::vector<Point> points1 = Generator::loadFromFile(path, sname);
      path = "../data/protein3D/W3.stp";
      sname = "W3";
      std::vector<Point> points2 = Generator::loadFromFile(path, sname);
    
      IterativeSmith is(MAX_DIM);
      ESMT e1(points1, &is, true, true, false);
      ESMT e2(points2, &is, true, true, false);
    
      Test test;
      std::cout << test.testTopology(e1,e2) << std::endl;
      return 0;
    }
    else if(c.is_set("counttest")) {
      int d = c.get_int_param("counttest",0),
	n = c.get_int_param("counttest",1),
	seed = c.get_int_param("counttest",2);
      Test test;
      test.doTestESMTSpecial(d, n, seed);
      return 0;
    }
#endif
    bool verbose = c.is_set("v");
    bool print   = c.is_set("pt");
    int n, dim;
    std::string algname, outfile, filename, settype, setname;
    SubgraphHeuristic *sh;
    SubgraphHeuristic *shsp = NULL;
    algname = c.is_set("alg") ? c.get_string_param("alg") : "NO";
    if(algname.compare(0, 10, "NO") == 0)
      sh = new IterativeSmith(MAX_DIM);
    else if(algname.compare(0, 10, "RNO") == 0)
      sh = new IterativeConcat(MAX_DIM);
    else if(algname.compare(0, 10, "SP") == 0) {
      shsp = new IterativeConcat(MAX_DIM);
      sh = new SteinerFinder(shsp);
    }
    else {
      c.error_usage("Unknown sub-graph algorithm");
      return 1;
    }
    
    // Heuristic parameters
    bool no_subgraph_concat = c.is_set("nsc"),
      no_post_optimise = c.is_set("npo"),
      redo_concat = c.is_set("rdc");
    int seed = c.is_set("s") ? c.get_int_param("s") : time(NULL);
    unsigned int use_bd = c.is_set("ubd") ? c.get_int_param("ubd") : 0,
      face_max_size = c.is_set("k") ? c.get_int_param("k") : 0;
    
    int p = c.get_chosen_pattern();
#ifdef ESMT_TEST_MODE
    if(p == PATTERN_BATCH && !c.is_set("batch")) {
      delete sh;
      delete shsp;
      c.error_usage("Please use --batch when no inputs given.");
    }
    else if(p == PATTERN_BATCH) {      
      // Check that some set is added
      if(!c.is_set("g") && !c.is_set("gn") && !c.is_set("if") && !c.is_set("ifa")) {
	delete sh;
	delete shsp;
	c.error_usage("No input set specified");
      }
      
      Test test;
      test.setSubgraphHeuristicOne(sh, algname);
      test.doConcatSubgraphs(!no_subgraph_concat);
      test.doPostOptimise(!no_post_optimise);
      test.doUseSpecialConcat(redo_concat);
      test.doUseBGraph(use_bd);
      test.setMaxFaceSize(face_max_size);
      test.inclDelaunay(!c.is_set("nd"));
      test.setLoopTime(c.is_set("lt") ? c.get_int_param("lt") : 0);
      test.setSeed(seed);
      test.doCollectStats(c.is_set("st"));
      outfile = c.is_set("out") ? c.get_string_param("out") : "";

      int t = -1;
      unsigned int i, num_g = c.is_set("g"), num_gn = c.is_set("gn"),
	num_if = c.is_set("if"), num_ifa = c.is_set("ifa");
      
      unsigned int numtests = 1;
      for(i = 0; i < num_g; i++) {
	settype  = c.get_string_param("g", 0, i);
	dim      = c.get_int_param("g", 1, i);
	n        = c.get_int_param("g", 2, i);
	t        = set_type_from_name(settype);
	if(t < 0)
	  c.error_usage("Unknown point set type");
	test.addTest(t, dim, n, numtests);
      }
      for(i = 0; i < num_gn; i++) {
	settype  = c.get_string_param("gn", 0, i);
	dim      = c.get_int_param("gn", 1, i);
	n        = c.get_int_param("gn", 2, i);
	numtests = c.get_int_param("gn", 3, i);
	t        = set_type_from_name(settype);
	if(t < 0)
	  c.error_usage("Unknown point set type");
	test.addTest(t, dim, n, numtests);
      }
      for(i = 0; i < num_if; i++) {
	filename = c.get_string_param("if", 0, i);
	setname  = c.get_string_param("if", 1, i);
	test.addTest(filename, setname);
      }
      for(i = 0; i < num_ifa; i++) {
	filename = c.get_string_param("ifa", 0, i);
	test.addTest(filename);
      }
      
      if(c.is_set("os"))
	test.doTestSubgraphAlgorithm(verbose);
      else
	test.doTestESMT(verbose);
      if(outfile.size() > 0)
	test.createDatFile(outfile);
    }
    else {
#endif
      std::vector<Point> points;
      if(p == PATTERN_IN_FILE) {
	filename = c.get_string_param("file");
	setname  = c.get_string_param("name");
	points = Generator::loadFromFile(filename,setname);
      }
      else if(p == PATTERN_GENERATE) {
	settype = c.get_string_param("type");
	dim     = c.get_int_param("d");
	n       = c.get_int_param("n");
	Generator::setSeed(seed);
	if(settype.compare(0, 20, "random") == 0)
	  points = Generator::randomFloatPoints(Point(dim, 100), Point(dim, -100), n);
	else if(settype.compare(0, 20, "simplex") == 0)
	  points = Generator::simplex(dim);
	else if(settype.compare(0, 20, "sausage") == 0)
	  points = Generator::sausage(dim, n);
	else if(settype.compare(0, 20, "grid") == 0)
	  points = Generator::gridFromSide(dim, n);
	else
	  c.error_usage("Unknown point set type");
      }
      
      ESMT esmt(points, sh, !no_subgraph_concat, !no_post_optimise,
		redo_concat, use_bd, face_max_size, verbose);
      
      if(c.is_set("val")) {
	if(Utils::validate(esmt))
	  std::cout << "Validate OK!" << std::endl;
	else
	  std::cout << "Validate ERROR!" << std::endl;
      }
      
      if(print)
	std::cout << "## RESULT ##" << std::endl << esmt << std::endl;
      else {
	std::cout << "Done!" << std::endl
		  << "  |MST| = " << esmt.getMSTLength() << std::endl
		  << "  |SMT| = " << esmt.getSMTLength() << std::endl
		  << "  Ratio = " << esmt.getSteinerRatio() << std::endl;
      }
#ifdef ESMT_TEST_MODE
    }
#endif
    
    delete sh;
    delete shsp;
    
    return 0;
  }
  catch(int e) { return e; }
}
