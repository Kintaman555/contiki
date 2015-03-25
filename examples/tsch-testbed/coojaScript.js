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
  //sim.stopSimulation();
  //var myNetwork = new java.lang.Array();
  //  means no link. 0 means 0 etx
  var numberOfNodes=10;
  var i=0;
  var j=0;
myNetwork=[0, 0.94828, 0.94828, 0.034483, 0, 0.94828, 0.94828, 0.68966, 0.94828, 0.94828, 0.98276, 0, 0.98276, 0, 0, 0.98276, 0, 0, 0.98276, 0.84483, 0.98276, 0.98276, 0, 0, 0, 0.051724, 0, 0, 0.98276, 0.98276, 0.068966, 0, 0, 0, 0.94828, 0.94828, 0.94828, 0.94828, 0, 0, 0, 0, 0, 0.94828, 0, 0, 0.94828, 0.94828, 0, 0, 0.94828, 0.93103, 0.017241, 0.94828, 0, 0, 0.94828, 0.2931, 0.81034, 0.94828, 0.96552, 0, 0, 0.96552, 0.96552, 0.96552, 0, 0.96552, 0, 0, 0.7931, 0, 0, 0.98276, 0.98276, 0.82759, 0.98276, 0, 0, 0, 0.98276, 0.98276, 0.98276, 0, 0, 0.98276, 0, 0, 0, 0, 0.94828, 0.93103, 0.94828, 0, 0, 0.94828, 0, 0, 0, 0];
//[0, 0.9, 0.9, 0.1, 0.1, 0.9, 0, 0.9, 0.8, 0.8, 0.9, 0.9, 0, 0.8, 0.8, 0.1, 0.8, 0.75, 0, 0.8, 0.1, 0.75, 0.8, 0.8, 0];
  
  
  //printEdges();          
  radioMedium = sim.getRadioMedium();
  if(radioMedium != null) {  
    radioMedium.clearEdges();
   
    for(i=0; i<numberOfNodes; i++) {
	    var srcRadio = sim.getMoteWithID(i+1).getInterfaces().getRadio();
      for(j=0; j<numberOfNodes; j++) {
	      var weight =  myNetwork[j*numberOfNodes + i];
	      if(i==j || weight == 0) {
          continue;
	      }
	      var ratio = weight;
	      var dstRadio = sim.getMoteWithID(j+1).getInterfaces().getRadio();
	      var superDest = new org.contikios.cooja.radiomediums.DGRMDestinationRadio(dstRadio);
	      superDest.ratio = ratio;
          var edge = new org.contikios.cooja.radiomediums.DirectedGraphMedium.Edge(srcRadio, superDest);
	      radioMedium.addEdge(edge);
    	}
    }
    
    printEdges();
    log.log("Script finished setting weights.\n"); 
  }
//sim.startSimulation();  
 TIMEOUT(360000);
 //import Java Package to JavaScript
 importPackage(java.io);
 date = new java.util.Date();
 // Use JavaScript object as an associative array
 path = sim.getCooja().getLastOpenedFile().getParent();
 outputFile = new FileWriter(path + "\/log_" + date.toString().replace(":", ".").replace(" ", "_") +".txt");

 while (true) {
 logMsg = time + "\tID:" + id + "\t" + msg + "\n";
    //Write to file.
    outputFile.write(logMsg);
    //log.log(logMsg);
    try{
        //This is the tricky part. The Script is terminated using
        // an exception. This needs to be caught.
        YIELD();
    } catch (e) {
        //Close files.
        outputFile.close();
        //Rethrow exception again, to end the script.
        throw('test script finished ' + time);
    }
 }