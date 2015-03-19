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
    <radiomedium>
      org.contikios.cooja.radiomediums.DirectedGraphMedium
      <edge>
        <source>1</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>2</radio>
          <ratio>1.0E-4</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>1</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>3</radio>
          <ratio>1.0E-4</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>1</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>4</radio>
          <ratio>1.0E-4</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>1</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>5</radio>
          <ratio>1.0E-4</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>2</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>1</radio>
          <ratio>0.5</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>2</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>3</radio>
          <ratio>0.5</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>2</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>4</radio>
          <ratio>1.0E-4</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>2</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>5</radio>
          <ratio>1.0E-4</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>3</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>1</radio>
          <ratio>0.5</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>3</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>2</radio>
          <ratio>0.5</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>3</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>4</radio>
          <ratio>1.0E-4</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>3</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>5</radio>
          <ratio>1.0E-4</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>4</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>1</radio>
          <ratio>0.1</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>4</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>2</radio>
          <ratio>0.3333333333333333</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>4</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>3</radio>
          <ratio>0.25</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>4</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>5</radio>
          <ratio>0.3333333333333333</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>5</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>1</radio>
          <ratio>0.1</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>5</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>2</radio>
          <ratio>0.25</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>5</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>3</radio>
          <ratio>0.3333333333333333</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>5</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>4</radio>
          <ratio>0.3333333333333333</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
    </radiomedium>
    <events>
      <logoutput>40000</logoutput>
    </events>
    <motetype>
      org.contikios.cooja.mspmote.SkyMoteType
      <identifier>sky1</identifier>
      <description>Sky Mote Type #sky1</description>
      <firmware EXPORT="copy">[CONFIG_DIR]/app-no-rpl-unicast.sky</firmware>
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
        <x>59.450233334362736</x>
        <y>39.89884141796077</y>
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
        <x>49.84119326898596</x>
        <y>59.62781851354264</y>
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
        <x>69.89096544196796</x>
        <y>59.5817423140423</y>
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
        <x>49.744052422102015</x>
        <y>79.73191500464084</y>
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
        <x>69.9522382564105</x>
        <y>79.77085871473714</y>
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
    <z>2</z>
    <height>160</height>
    <location_x>400</location_x>
    <location_y>0</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.Visualizer
    <plugin_config>
      <moterelations>true</moterelations>
      <skin>org.contikios.cooja.plugins.skins.IDVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.GridVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.DGRMVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.TrafficVisualizerSkin</skin>
      <viewport>4.638084462195865 0.0 0.0 4.638084462195865 -30.080752989244314 -100.66456679908072</viewport>
    </plugin_config>
    <width>400</width>
    <z>6</z>
    <height>400</height>
    <location_x>1</location_x>
    <location_y>1</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.LogListener
    <plugin_config>
      <filter>ping</filter>
      <formatted_time />
      <coloring />
    </plugin_config>
    <width>1520</width>
    <z>4</z>
    <height>240</height>
    <location_x>400</location_x>
    <location_y>160</location_y>
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
      <zoomfactor>143.0115598307874</zoomfactor>
    </plugin_config>
    <width>1920</width>
    <z>3</z>
    <height>166</height>
    <location_x>0</location_x>
    <location_y>962</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.Notes
    <plugin_config>
      <notes>Enter notes here</notes>
      <decorations>true</decorations>
    </plugin_config>
    <width>1240</width>
    <z>5</z>
    <height>160</height>
    <location_x>680</location_x>
    <location_y>0</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>function printEdges() {   
      radioMedium = sim.getRadioMedium();
	  if(radioMedium != null) {  
	    radios = radioMedium.getRegisteredRadios();
	    edges = radioMedium.getEdges();
	    if(edges != null) {
            log.log("Edges:\n");
		    for(i=0; i&lt;edges.length; i++) {
		        str = java.lang.String.format("%d -&gt; %d, ratio: %.2f\n", new java.lang.Integer(edges[i].source.getMote().getID()), new java.lang.Integer(edges[i].superDest.radio.getMote().getID()), new java.lang.Float(edges[i].superDest.ratio));
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
  myNetwork=[0, 10000, 10000, 10000, 10000, 2, 0, 2, 10000, 10000, 2, 2, 0, 10000, 10000, 10, 3, 4, 0, 3, 10, 4, 3, 3, 0];
  
  //printEdges();          
  radioMedium = sim.getRadioMedium();
  if(radioMedium != null) {  
    sim.stopSimulation();
    radioMedium.clearEdges();
   
    for(i=0; i&lt;numberOfNodes; i++) {
	    var srcRadio = sim.getMoteWithID(i+1).getInterfaces().getRadio();
      for(j=0; j&lt;numberOfNodes; j++) {
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
  }</script>
      <active>true</active>
    </plugin_config>
    <width>810</width>
    <z>1</z>
    <height>700</height>
    <location_x>489</location_x>
    <location_y>140</location_y>
  </plugin>
</simconf>

