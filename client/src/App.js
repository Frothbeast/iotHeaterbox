import React, { useState, useMemo, useEffect } from 'react';
import { useHeaterData } from './hooks/useHeaterData';
import HeaterSidebar from './components/sidebar/heaterSidebar';
import HeaterTable from './components/HeaterTable/HeaterTable';
import ControlBar from './components/ControlBar/ControlBar';
import { calculateColumnStats  } from './utils/heaterStats';
import './App.css';

function App() {
  const [selectedHours, setSelectedHours] = useState(24);
  const [isSidebarOpen, setSidebarOpen] = useState(false);
  
  const { heaterRecords, isLoading } = useHeaterData(selectedHours);

  const columnStats = useMemo(() => calculateColumnStats (heaterRecords), [heaterRecords]);

  const [serverTime, setServerTime] = useState("00:00 AM");

  const cl1pClick = async () => {
    try{
      const response = await fetch(`${process.env.REACT_APP_HEATERBOX_API_URL}/api/cl1p`, {method: 'POST', });
      if (response.status === 204) {
          console.log("Action acknowledged by server.");
      }
      else {
        console.error("Server returned an error");
      }
    }
    catch (error) {
      console.error("Fetch error:", error);
    }
  };

  const updateTime = () => {
    fetch(`${process.env.REACT_APP_HEATERBOX_API_URL}/api/time`)
      .then(res => res.json())
      .then(data => setServerTime(data.time))
      .catch(err => console.error("Time fetch failed", err));
  };

  useEffect(() => {
    updateTime();
    const interval = setInterval(updateTime, 60000);
    return () => clearInterval(interval);
  }, []);

  if (isLoading) return <div className="loader">Loading Heater Box Data...</div>;

  return (
    <div className="App">
      <ControlBar
        selectedHours={selectedHours}
        onHoursChange={setSelectedHours}
        columnStats={columnStats}
        cl1pClick={cl1pClick}
        records={heaterRecords}
        toggleSidebar={() => setSidebarOpen(!isSidebarOpen)}
        isSidebarOpen={isSidebarOpen}
        serverTime={serverTime}
      />
      <main>
        <HeaterSidebar 
          isOpen={isSidebarOpen} 
          records={heaterRecords}
          selectedHours={selectedHours} 
        />
        <HeaterTable 
          records={heaterRecords} 
          columnStats={columnStats} 
        />
      </main>
    </div>
  );
}

export default App;
