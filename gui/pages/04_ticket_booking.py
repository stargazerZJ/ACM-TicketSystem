import datetime
import streamlit as st
from streamlit_server_state import server_state, server_state_lock
from utils.helpers import ensure_api_client, ensure_username

# Initialize global ApiClient instance
ensure_api_client()
ensure_username()

def query_ticket_form():
    st.session_state['ticket_page'] = 'Query Ticket'
    st.title("Query Tickets")

    # Form for querying tickets
    with st.form("query_ticket_form"):
        start_station = st.text_input("Start Station", value=st.session_state.get("start_station", "Station_1"))
        end_station = st.text_input("End Station", value=st.session_state.get("end_station", "Station_2"))
        travel_date = st.date_input("Travel Date", value=st.session_state.get("travel_date", datetime.date(2024, 6, 1)),
                                    min_value=datetime.date(2024, 6, 1), max_value=datetime.date(2024, 8, 31))
        sort_option = st.selectbox("Sort By", ["Time", "Cost"], index=["Time", "Cost"].index(st.session_state.get("sort_option", "Time")))
        transfer_option = st.checkbox("Query Transfer", value=st.session_state.get("transfer_option", False))
        submit_button = st.form_submit_button("Query Ticket")

    if submit_button:
        # Store query parameters in session state
        st.session_state["start_station"] = start_station
        st.session_state["end_station"] = end_station
        st.session_state["travel_date"] = travel_date
        st.session_state["sort_option"] = sort_option
        st.session_state["transfer_option"] = transfer_option

        with server_state_lock["api_client"]:
            api_client = server_state.api_client
            sort_param = 'time' if sort_option == "Time" else 'cost'

            if transfer_option:
                result = api_client.query_transfer(start_station, end_station, travel_date.strftime('%m-%d'), sort_param)
            else:
                result = api_client.query_ticket(start_station, end_station, travel_date.strftime('%m-%d'), sort_param)

            if result:
                result_lines = result.strip().split("\n")
                if result_lines[0] == "0":
                    st.write("No matching trains found.")
                else:
                    num_trains = int(result_lines[0])
                    st.write(f"Number of matching trains: {num_trains}")

                    # Store results in session state to persist across reruns
                    st.session_state['query_results'] = result_lines[1:]

    # Check if query results exist in session state
    if 'query_results' in st.session_state:
        for i, line in enumerate(st.session_state['query_results'], 1):
            train_info = line.split()
            st.write(
                f"Train ID: {train_info[0]}, From: {train_info[1]}, Leaving Time: {train_info[2]} {train_info[3]} -> To: {train_info[5]}, Arriving Time: {train_info[6]} {train_info[7]}, Price: {train_info[8]}, Seat: {train_info[9]}")

            # Use a unique key for each button and handle clicks via callback
            def buy_ticket_callback(train_info=train_info):
                print(f"Buying ticket for train {train_info[0]}")
                st.session_state['buy_params'] = {
                    'trainID': train_info[0],
                    'start_station': train_info[1],
                    'end_station': train_info[5],
                    'travel_date': travel_date,
                    'num_tickets': 1,
                    'accept_standby': False
                }
                st.session_state['ticket_page'] = 'Buy Ticket'
                print("session_state: ", st.session_state)

            st.button("Buy Ticket", key=f"buy_button_{i}", on_click=buy_ticket_callback)

def buy_ticket_form():
    st.session_state['ticket_page'] = 'Buy Ticket'
    st.title("Buy Tickets")
    params = st.session_state.get('buy_params', {
        'trainID': 'Train_1',
        'start_station': 'Station_1',
        'end_station': 'Station_2',
        'travel_date': datetime.date(2024, 6, 1),
        'num_tickets': 1,
        'accept_standby': False
    })

    trainID = st.text_input("Train ID", value=params['trainID'])
    start_station = st.text_input("Start Station", value=params['start_station'])
    end_station = st.text_input("End Station", value=params['end_station'])
    travel_date = st.date_input("Travel Date", value=params['travel_date'],
                                min_value=datetime.date(2024, 6, 1), max_value=datetime.date(2024, 8, 31))
    num_tickets = st.number_input("Number of Tickets", min_value=1, step=1, value=params['num_tickets'])
    accept_standby = st.checkbox("Accept Standby", value=params['accept_standby'])
    submit_button = st.button("Buy Ticket")

    if submit_button:
        with server_state_lock["api_client"]:
            api_client = server_state.api_client
            username = st.session_state.get('username', 'guest')
            result = api_client.buy_ticket(username, trainID, travel_date.strftime('%m-%d'), num_tickets, start_station,
                                           end_station, accept_standby)
            if result == "-1":
                st.error("Purchase failed. Please check the details.")
            elif result == "queue":
                st.warning("Added to standby queue.")
            else:
                st.success(f"Purchase successful. Total price: {result}")


def app():
    st.title("Ticket Booking")
    page = st.session_state.get('ticket_page', 'Query Ticket')
    # print("session_state: ", st.session_state)
    option = st.selectbox("Select Functionality", ["Query Ticket", "Buy Ticket"], index=0 if page == 'Query Ticket' else 1)

    if option == "Query Ticket":
        query_ticket_form()
    elif option == "Buy Ticket":
        buy_ticket_form()


if __name__ == "__main__":
    app()
