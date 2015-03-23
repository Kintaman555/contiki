<?xml version="1.0" encoding="UTF-8"?>
<simconf>
  <project EXPORT="discard">[APPS_DIR]/mrm</project>
  <project EXPORT="discard">[APPS_DIR]/mspsim</project>
  <project EXPORT="discard">[APPS_DIR]/avrora</project>
  <project EXPORT="discard">[APPS_DIR]/serial_socket</project>
  <project EXPORT="discard">[APPS_DIR]/collect-view</project>
  <project EXPORT="discard">[APPS_DIR]/powertracker</project>
  <simulation>
    <title>My simulation</title>
    <randomseed>123456</randomseed>
    <motedelay_us>1000000</motedelay_us>
    <radiomedium>org.contikios.cooja.radiomediums.DirectedGraphMedium</radiomedium>
    <events>
      <logoutput>40000</logoutput>
    </events>
    <motetype>
      org.contikios.cooja.mspmote.SkyMoteType
      <identifier>sky1</identifier>
      <description>collect-only</description>
      <firmware EXPORT="copy">[CONTIKI_DIR]/examples/tsch-testbed/app-rpl-collect-only.sky</firmware>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyButton</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyFlash</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyCoffeeFilesystem</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.Msp802154Radio</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspSerial</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyLED</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDebugOutput</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.SkyTemperature</moteinterface>
    </motetype>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>39.0872828811514</x>
        <y>18.711115055434973</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>1</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>30.27225325372137</x>
        <y>39.450685592041985</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>2</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>49.64120150105368</x>
        <y>39.55109842586455</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>3</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>30.229905750736947</x>
        <y>59.915681579522435</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>4</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>49.80000945742036</x>
        <y>59.78868510386111</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>5</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.SimControl
    <width>280</width>
    <z>0</z>
    <height>160</height>
    <location_x>1</location_x>
    <location_y>456</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.Visualizer
    <plugin_config>
      <moterelations>true</moterelations>
      <skin>org.contikios.cooja.plugins.skins.IDVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.GridVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.DGRMVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.TrafficVisualizerSkin</skin>
      <viewport>4.100509291950746 0.0 0.0 4.100509291950746 -6.1178880384803875 -15.452373874843353</viewport>
    </plugin_config>
    <width>400</width>
    <z>1</z>
    <height>400</height>
    <location_x>1</location_x>
    <location_y>1</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.LogListener
    <plugin_config>
      <filter />
      <formatted_time />
      <coloring />
    </plugin_config>
    <width>601</width>
    <z>3</z>
    <height>389</height>
    <location_x>401</location_x>
    <location_y>293</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.TimeLine
    <plugin_config>
      <mote>0</mote>
      <mote>1</mote>
      <mote>2</mote>
      <mote>3</mote>
      <mote>4</mote>
      <showRadioRXTX />
      <showRadioHW />
      <showLEDs />
      <zoomfactor>500.0</zoomfactor>
    </plugin_config>
    <width>1913</width>
    <z>2</z>
    <height>234</height>
    <location_x>3</location_x>
    <location_y>682</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>function printEdges() {   &#xD;
      radioMedium = sim.getRadioMedium();&#xD;
	  if(radioMedium != null) {  &#xD;
	    radios = radioMedium.getRegisteredRadios();&#xD;
	    edges = radioMedium.getEdges();&#xD;
	    if(edges != null) {&#xD;
            log.log("Edges:\n");&#xD;
		    for(i=0; i&lt;edges.length; i++) {&#xD;
		        str = java.lang.String.format("%d -&gt; %d, ratio: %.2f\n", new java.lang.Integer(edges[i].source.getMote().getID()), new java.lang.Integer(edges[i].superDest.radio.getMote().getID()), new java.lang.Float(edges[i].superDest.ratio));&#xD;
			    log.log(str);&#xD;
		    }&#xD;
	    } &#xD;
    }&#xD;
  }&#xD;
  log.log("Script started.\n");&#xD;
  //sim.stopSimulation();&#xD;
  //var myNetwork = new java.lang.Array();&#xD;
  //  means no link. 0 means 0 etx&#xD;
  var numberOfNodes=5;&#xD;
  var i=0;&#xD;
  var j=0;&#xD;
myNetwork=[0, 0.9, 0.9, 0.1, 0.1, 0.9, 0, 0.9, 0.8, 0.8, 0.9, 0.9, 0, 0.8, 0.8, 0.1, 0.8, 0.75, 0, 0.8, 0.1, 0.75, 0.8, 0.8, 0];&#xD;
  &#xD;
  &#xD;
  //printEdges();          &#xD;
  radioMedium = sim.getRadioMedium();&#xD;
  if(radioMedium != null) {  &#xD;
    radioMedium.clearEdges();&#xD;
   &#xD;
    for(i=0; i&lt;numberOfNodes; i++) {&#xD;
	    var srcRadio = sim.getMoteWithID(i+1).getInterfaces().getRadio();&#xD;
      for(j=0; j&lt;numberOfNodes; j++) {&#xD;
	      var weight =  myNetwork[i*numberOfNodes + j];&#xD;
	      if(i==j || weight == 0) {&#xD;
          continue;&#xD;
	      }&#xD;
	      var ratio = weight;&#xD;
	      var dstRadio = sim.getMoteWithID(j+1).getInterfaces().getRadio();&#xD;
	      var superDest = new org.contikios.cooja.radiomediums.DGRMDestinationRadio(dstRadio);&#xD;
	      superDest.ratio = ratio;&#xD;
          var edge = new org.contikios.cooja.radiomediums.DirectedGraphMedium.Edge(srcRadio, superDest);&#xD;
	      radioMedium.addEdge(edge);&#xD;
    	}&#xD;
    }&#xD;
    &#xD;
    printEdges();&#xD;
    log.log("Script finished setting weights.\n"); &#xD;
  }&#xD;
//sim.startSimulation();  &#xD;
 TIMEOUT(3600000);&#xD;
 //import Java Package to JavaScript&#xD;
 importPackage(java.io);&#xD;
 date = new java.util.Date();&#xD;
 // Use JavaScript object as an associative array&#xD;
 path = sim.getCooja().getLastOpenedFile().getParent();&#xD;
 outputFile = new FileWriter(path + "\/log_" + date.toString().replace(":", ".").replace(" ", "_") +".txt");&#xD;
&#xD;
 while (true) {&#xD;
 logMsg = time + "\tID:" + id + "\t" + msg + "\n";&#xD;
    //Write to file.&#xD;
    outputFile.write(logMsg);&#xD;
    //log.log(logMsg);&#xD;
    try{&#xD;
        //This is the tricky part. The Script is terminated using&#xD;
        // an exception. This needs to be caught.&#xD;
        YIELD();&#xD;
    } catch (e) {&#xD;
        //Close files.&#xD;
        outputFile.close();&#xD;
        //Rethrow exception again, to end the script.&#xD;
        throw('test script finished ' + time);&#xD;
    }&#xD;
 }</script>
      <active>true</active>
    </plugin_config>
    <width>921</width>
    <z>4</z>
    <height>683</height>
    <location_x>999</location_x>
    <location_y>0</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.RadioLogger
    <plugin_config>
      <split>150</split>
      <formatted_time />
      <showdups>false</showdups>
      <hidenodests>false</hidenodests>
    </plugin_config>
    <width>597</width>
    <z>5</z>
    <height>292</height>
    <location_x>403</location_x>
    <location_y>1</location_y>
  </plugin>
</simconf>

