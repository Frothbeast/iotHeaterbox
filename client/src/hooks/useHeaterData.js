import { useState, useEffect } from 'react';

export function useHeaterData(hours) {
    const [heaterRecords, setHeaterRecords] = useState([]);
    const [isLoading, setIsLoading] = useState(true);

    useEffect(() => {
        let interval;

        const fetchData = () => {
            fetch(`${process.env.REACT_APP_HEATER_API_URL}/api/heaterData?hours=${hours}`)
                .then(res => res.json())
                .then(data => {
                    if (Array.isArray(data)) {
                        setHeaterRecords(data); //
                    }
                    setIsLoading(false);
                })
                .catch(err => {
                    console.error("Fetch error:", err);
                    setIsLoading(false);
                });
        };

        const setupInterval = () => {
            if (interval) clearInterval(interval);
            const pollRate = document.visibilityState === 'visible' ? 5000 : 60000;
            interval = setInterval(fetchData, pollRate);
        };

        fetchData();
        setupInterval();

        const handleVisibilityChange = () => setupInterval();
        document.addEventListener('visibilitychange', handleVisibilityChange);

        return () => {
            clearInterval(interval);
            document.removeEventListener('visibilitychange', handleVisibilityChange);
        };
    }, [hours]);

    return { heaterRecords, isLoading };
}