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
          <ratio>0.068966</ratio>
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
          <ratio>0.96552</ratio>
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
          <ratio>0.94828</ratio>
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
          <ratio>0.94828</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>1</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>6</radio>
          <ratio>0.94828</ratio>
          <signal>-10.0</signal>
          <lqi>105</lqi>
          <delay>0</delay>
        </dest>
      </edge>
      <edge>
        <source>1</source>
        <dest>
          org.contikios.cooja.radiomediums.DGRMDestinationRadio
          <radio>7</radio>
          <ratio>0.96552</ratio>
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
        <x>39.84726195971331</x>
        <y>29.097495795780954</y>
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
        <x>29.258947815638834</x>
        <y>29.317631211216632</y>
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
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>63.99283517970595</x>
        <y>47.47517169555054</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>6</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>13.834472773379808</x>
        <y>47.5478425752644</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>7</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>39.916646282596844</x>
        <y>70.23696109377504</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>8</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>24.841937158557272</x>
        <y>49.96984810346475</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>9</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>39.916896549641194</x>
        <y>49.820997930977924</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>10</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>63.342014606274134</x>
        <y>10.327637093658893</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>11</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>36.020235313504</x>
        <y>78.12641889100604</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>12</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>7.17637973014379</x>
        <y>82.1710879496541</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>13</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>45.73807802251157</x>
        <y>22.746557561778136</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>14</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>30.58106138896989</x>
        <y>6.2383833051532385</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>15</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>78.70154890939017</x>
        <y>92.61464098565348</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>16</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>22.440497032473573</x>
        <y>34.18925755266281</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>17</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>46.615113208858446</x>
        <y>90.49012812121086</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>18</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>68.66192602224088</x>
        <y>6.530340398336065</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>19</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>84.38402794527605</x>
        <y>62.67962508069198</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>20</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>65.115763259921</x>
        <y>96.72841483886052</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>21</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>78.3486646813379</x>
        <y>37.41035586662783</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>22</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>55.07369234205519</x>
        <y>26.10421133740992</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>23</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>91.09206189286196</x>
        <y>40.31370035050329</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>24</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>20.483187027521467</x>
        <y>87.18877125572678</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>25</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>56.69778136117131</x>
        <y>19.289495838511638</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>26</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>55.2029602244427</x>
        <y>77.33308840141116</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>27</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>32.74084551154346</x>
        <y>72.41743577933086</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>28</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>34.139656655460804</x>
        <y>43.49842948935893</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>29</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>85.99012301930814</x>
        <y>76.28100524989503</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.mspmote.interfaces.MspMoteID
        <id>30</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.SimControl
    <width>280</width>
    <z>2</z>
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
      <viewport>3.4760232615046354 0.0 0.0 3.4760232615046354 23.208305523310827 -5.957492755622031</viewport>
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
    <z>5</z>
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
      <mote>5</mote>
      <mote>6</mote>
      <mote>7</mote>
      <mote>8</mote>
      <mote>9</mote>
      <mote>10</mote>
      <mote>11</mote>
      <mote>12</mote>
      <mote>13</mote>
      <mote>14</mote>
      <mote>15</mote>
      <mote>16</mote>
      <mote>17</mote>
      <mote>18</mote>
      <mote>19</mote>
      <mote>20</mote>
      <mote>21</mote>
      <mote>22</mote>
      <mote>23</mote>
      <mote>24</mote>
      <mote>25</mote>
      <mote>26</mote>
      <mote>27</mote>
      <mote>28</mote>
      <mote>29</mote>
      <showRadioRXTX />
      <showRadioHW />
      <showLEDs />
      <zoomfactor>1000.0</zoomfactor>
    </plugin_config>
    <width>1913</width>
    <z>4</z>
    <height>234</height>
    <location_x>3</location_x>
    <location_y>682</location_y>
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
  //sim.stopSimulation();
  //var myNetwork = new java.lang.Array();
  //  means no link. 0 means 0 etx
  
  var i=0;
  var j=0;
//myNetwork=[0, 0.94828, 0.94828, 0.034483, 0, 0.94828, 0.94828, 0.68966, 0.94828, 0.94828, 0.98276, 0, 0.98276, 0, 0, 0.98276, 0, 0, 0.98276, 0.84483, 0.98276, 0.98276, 0, 0, 0, 0.051724, 0, 0, 0.98276, 0.98276, 0.068966, 0, 0, 0, 0.94828, 0.94828, 0.94828, 0.94828, 0, 0, 0, 0, 0, 0.94828, 0, 0, 0.94828, 0.94828, 0, 0, 0.94828, 0.93103, 0.017241, 0.94828, 0, 0, 0.94828, 0.2931, 0.81034, 0.94828, 0.96552, 0, 0, 0.96552, 0.96552, 0.96552, 0, 0.96552, 0, 0, 0.7931, 0, 0, 0.98276, 0.98276, 0.82759, 0.98276, 0, 0, 0, 0.98276, 0.98276, 0.98276, 0, 0, 0.98276, 0, 0, 0, 0, 0.94828, 0.93103, 0.94828, 0, 0, 0.94828, 0, 0, 0, 0];
//[0, 0.9, 0.9, 0.1, 0.1, 0.9, 0, 0.9, 0.8, 0.8, 0.9, 0.9, 0, 0.8, 0.8, 0.1, 0.8, 0.75, 0, 0.8, 0.1, 0.75, 0.8, 0.8, 0];
  /* PRR for cooja */ myNetwork = [0, 0.034483, 0.94828, 0.94828, 0.94828, 0.94828, 0.94828, 0, 0, 0, 0.94828, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.77586, 0, 0, 0, 0, 0, 0, 0, 0.068966, 0, 0.94828, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.62069, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0.96552, 0, 0, 0, 0, 0, 0, 0, 0, 0.086207, 0, 0, 0, 0, 0, 0, 0.96552, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.94828, 0, 0, 0, 0.94828, 0, 0.94828, 0, 0, 0, 0.017241, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.94828, 0, 0, 0.94828, 0, 0, 0.94828, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.94828, 0, 0, 0, 0, 0, 0.31034, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.93103, 0, 0.94828, 0, 0, 0, 0, 0, 0.96552, 0, 0, 0.96552, 0.96552, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0, 0.96552, 0.96552, 0, 0.96552, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.94828, 0, 0, 0.94828, 0.94828, 0, 0.94828, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.12069, 0, 0, 0, 0, 0, 0.96552, 0, 0.91379, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.98276, 0, 0.10345, 0, 0, 0, 0, 0.98276, 0.98276, 0.27586, 0, 0.98276, 0, 0, 0, 0, 0.67241, 0, 0.84483, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0.96552, 0, 0.96552, 0, 0, 0.96552, 0, 0, 0, 0.96552, 0.034483, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.94828, 0.94828, 0, 0.93103, 0, 0.87931, 0, 0, 0, 0, 0.86207, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.94828, 0.94828, 0, 0, 0.94828, 0.94828, 0, 0.034483, 0, 0.89655, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.98276, 0.034483, 0, 0, 0.87931, 0, 0.82759, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0.7931, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0.7069, 0, 0.96552, 0.91379, 0.86207, 0, 0, 0, 0.94828, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.89655, 0, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0, 0, 0, 0.96552, 0, 0, 0, 0, 0, 0, 0, 0.87931, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0.91379, 0, 0.94828, 0, 0.65517, 0, 0.96552, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.98276, 0.98276, 0.98276, 0, 0, 0, 0.75862, 0, 0.98276, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0, 0, 0, 0.86207, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0, 0, 0.017241, 0, 0.96552, 0.96552, 0, 0, 0, 0, 0.82759, 0.068966, 0, 0, 0, 0.7931, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.98276, 0, 0, 0, 0, 0.017241, 0, 0.96552, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.93103, 0, 0, 0.98276, 0, 0.98276, 0, 0, 0.96552, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.96552, 0.017241, 0, 0, 0, 0, 0.10345, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.98276, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.93103, 0, 0, 0.98276, 0, 0, 0, 0, 0.98276, 0.98276, 0.87931, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.24138, 0, 0.98276, 0, 0.98276, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.93103, 0, 0, 0, 0, 0, 0, 0.93103, 0.93103, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.67241, 0, 0, 0, ]; 
var numberOfNodes=java.lang.Math.sqrt(myNetwork.length);
  
  //printEdges();          
  radioMedium = sim.getRadioMedium();
  if(radioMedium != null) {  
    radioMedium.clearEdges();
   
    for(i=0; i&lt;numberOfNodes; i++) {
	    var srcRadio = sim.getMoteWithID(i+1).getInterfaces().getRadio();
      for(j=0; j&lt;numberOfNodes; j++) {
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
 TIMEOUT(720000);
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
 }</script>
      <active>false</active>
    </plugin_config>
    <width>921</width>
    <z>0</z>
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
    <z>3</z>
    <height>292</height>
    <location_x>403</location_x>
    <location_y>1</location_y>
  </plugin>
</simconf>

