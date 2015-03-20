  function printEdges() {   
      radioMedium = sim.getRadioMedium();
	  if(radioMedium != null) {  
	    radios = radioMedium.getRegisteredRadios();
	    edges = radioMedium.getEdges();
	    if(edges != null) {
            log.log("Edges:\n");
		    for(i=0; i<edges.length; i++) {
		        str = java.lang.String.format("%d -> %d, ratio: %.2f\n", new java.lang.Integer(edges[i].source.getMote().getID()), new java.lang.Integer(edges[i].superDest.radio.getMote().getID()), new java.lang.Float(edges[i].superDest.ratio));
			    log.log(str);
		    }
	    } 
    }
  }
  log.log("Script started.\n");
  //var myNetwork = new java.lang.Array();
  //  means no link. 0 means 0 etx
  var numberOfNodes=5;
  var i=0;
  var j=0;
  myNetwork=[0, 1, 1, 1, 1, 2, 0, 2, 1, 1, 2, 2, 0, 1, 1, 10, 3, 4, 0, 3, 10, 4, 3, 3, 0];
  
  //printEdges();          
  radioMedium = sim.getRadioMedium();
  if(radioMedium != null) {  
    sim.stopSimulation();
    radioMedium.clearEdges();
   
    for(i=0; i<numberOfNodes; i++) {
	    var srcRadio = sim.getMoteWithID(i+1).getInterfaces().getRadio();
      for(j=0; j<numberOfNodes; j++) {
	      var weight =  myNetwork[i*numberOfNodes + j];
	      if(i==j || weight == 0) {
          continue;
	      }
	      var ratio = 1.0/weight;
	      var dstRadio = sim.getMoteWithID(j+1).getInterfaces().getRadio();
	      var superDest = new org.contikios.cooja.radiomediums.DGRMDestinationRadio(dstRadio);
	      superDest.ratio = ratio;
          var edge = new org.contikios.cooja.radiomediums.DirectedGraphMedium.Edge(srcRadio, superDest);
	      radioMedium.addEdge(edge);
    	}
    }
    sim.startSimulation();
    
    printEdges();
    log.log("Script finished.\n"); 
  }
