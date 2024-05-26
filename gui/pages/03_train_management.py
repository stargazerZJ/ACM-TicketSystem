import datetime

import streamlit as st
from streamlit_server_state import server_state, server_state_lock
from utils.helpers import ensure_api_client

# Initialize global ApiClient instance
ensure_api_client()


def app():
    st.title("Train Management")

    option = st.selectbox("Select Functionality", ["Add Train", "Manage Existing Train"])

    if option == "Add Train":
        add_train_form()
    elif option == "Manage Existing Train":
        manage_train_form()



def add_train_form():
    with st.form("add_train_form"):
        trainID = st.text_input("Train ID", value="Train_1")
        seatNum = st.number_input("Number of Seats", min_value=1, value=100)
        startTime = st.time_input("Start Time", value=datetime.time(8, 0), step=datetime.timedelta(minutes=1))

        # Improved sale date input
        col1, col2 = st.columns(2)
        with col1:
            saleStartDate = st.date_input("Sale Start Date", value=datetime.date(2024, 6, 1),
                                          min_value=datetime.date(2024, 6, 1), max_value=datetime.date(2024, 8, 31))
        with col2:
            saleEndDate = st.date_input("Sale End Date", value=datetime.date(2024, 8, 31),
                                        min_value=datetime.date(2024, 6, 1), max_value=datetime.date(2024, 8, 31))

        trainType = st.text_input("Train Type (single uppercase letter)", value='G').upper()

        # Initialize session state for dynamic station fields
        if 'stations' not in st.session_state:
            st.session_state.stations = ["Station_1", "Station_2"]
            st.session_state.prices = [100]
            st.session_state.travelTimes = [60]
            st.session_state.stopoverTimes = ["_"] if len(st.session_state.stations) == 2 else [5] * (
                    len(st.session_state.stations) - 1)

        # Display current stations table
        st.write("### Stations and Details")
        for i in range(len(st.session_state.stations)):
            cols = st.columns(4)
            st.session_state.stations[i] = cols[0].text_input(f"Station {i + 1}", value=st.session_state.stations[i],
                                                              key=f"station_{i}")
            if i < len(st.session_state.prices):
                st.session_state.prices[i] = cols[1].number_input(f"Price to next",
                                                                  value=int(st.session_state.prices[i]),
                                                                  key=f"price_{i}")
                st.session_state.travelTimes[i] = cols[2].number_input(f"Travel Time to next (min)",
                                                                       value=int(st.session_state.travelTimes[i]),
                                                                       key=f"travelTime_{i}")
            if len(st.session_state.stopoverTimes) > i > 0 and len(st.session_state.stations) > 2:
                st.session_state.stopoverTimes[i] = cols[3].number_input(f"Stopover Time (min)",
                                                                         value=int(st.session_state.stopoverTimes[i]),
                                                                         key=f"stopoverTime_{i}")

        cols = st.columns(2)

        add_station = cols[0].form_submit_button("Add Station")
        if add_station:
            st.session_state.stations.append(f"Station_{len(st.session_state.stations) + 1}")
            if len(st.session_state.prices) < len(st.session_state.stations) - 1:
                st.session_state.prices.append(100)
                st.session_state.travelTimes.append(60)
            if len(st.session_state.stopoverTimes) < len(st.session_state.stations) - 1:
                st.session_state.stopoverTimes.append(5)
            st.rerun()

        if len(st.session_state.stations) > 2:
            remove_station = cols[1].form_submit_button("Remove Last Station")
            if remove_station and len(st.session_state.stations) > 2:
                st.session_state.stations.pop()
                if len(st.session_state.prices) >= len(st.session_state.stations):
                    st.session_state.prices.pop()
                    st.session_state.travelTimes.pop()
                if len(st.session_state.stopoverTimes) >= len(st.session_state.stations) - 1:
                    st.session_state.stopoverTimes.pop()
                st.rerun()

        submitted = st.form_submit_button("Add Train")

        if submitted:
            with server_state_lock["api_client"]:
                api_client = server_state.api_client
                stationNum = len(st.session_state.stations)
                stations = "|".join([station.strip() for station in st.session_state.stations if station.strip()])
                prices = "|".join([str(price) for price in st.session_state.prices])
                travelTimes = "|".join([str(time) for time in st.session_state.travelTimes])
                stopoverTimes = "|".join([str(time) for time in st.session_state.stopoverTimes if time != "_"])
                if stationNum == 2: # No stopover times for 2 stations
                    stopoverTimes = "_"
                saleDate = f"{saleStartDate.strftime('%m-%d')}|{saleEndDate.strftime('%m-%d')}"
                result = api_client.add_train(trainID, stationNum, seatNum, stations, prices,
                                              startTime.strftime('%H:%M'), travelTimes, stopoverTimes, saleDate,
                                              trainType)
                if result == "0":
                    st.success("Train added successfully.")
                else:
                    st.error("Failed to add train. The train ID might already exist.")


def manage_train_form():
    with st.form("manage_train_form"):
        trainID = st.text_input("Train ID", value="Train_1")
        queryDate = st.date_input("Query Date", value=datetime.date(2024, 6, 1),
                                  min_value=datetime.date(2024, 6, 1), max_value=datetime.date(2024, 8, 31))

        # Buttons for different functionalities
        col1, col2, col3 = st.columns(3)
        queryButton = col1.form_submit_button("Query Train")
        releaseButton = col2.form_submit_button("Release Train")
        deleteButton = col3.form_submit_button("Delete Train")

        if queryButton:
            with server_state_lock["api_client"]:
                api_client = server_state.api_client
                result = api_client.query_train(trainID, queryDate.strftime('%m-%d'))
                if result != "-1":
                    st.text(result)
                else:
                    st.error("Failed to query train. The train ID or date might be incorrect.")

        if releaseButton:
            with server_state_lock["api_client"]:
                api_client = server_state.api_client
                result = api_client.release_train(trainID)
                if result == "0":
                    st.success(f"Train {trainID} released successfully.")
                else:
                    st.error("Failed to release train. The train ID might be incorrect or train is already released.")

        if deleteButton:
            with server_state_lock["api_client"]:
                api_client = server_state.api_client
                result = api_client.delete_train(trainID)
                if result == "0":
                    st.success(f"Train {trainID} deleted successfully.")
                else:
                    st.error("Failed to delete train. The train ID might be incorrect or train cannot be deleted.")


if __name__ == "__main__":
    app()
