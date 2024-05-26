import datetime
import streamlit as st
from streamlit_server_state import server_state, server_state_lock
from utils.helpers import ensure_api_client, ensure_username

ensure_api_client()
ensure_username()


def query_ticket_form():
    with st.form("query_ticket_form"):
        start_station = st.text_input("Start Station", value="Station_1")
        end_station = st.text_input("End Station", value="Station_2")
        travel_date = st.date_input("Travel Date", value=datetime.date(2024, 6, 1),
                                    min_value=datetime.date(2024, 6, 1), max_value=datetime.date(2024, 8, 31))
        sort_option = st.selectbox("Sort By", ["Time", "Cost"])
        transfer_option = st.checkbox("Query Transfer")
        submit_button = st.form_submit_button("Query Ticket")

        if submit_button:
            with server_state_lock["api_client"]:
                api_client = server_state.api_client
                sort_param = 'time' if sort_option == "Time" else 'cost'

                if transfer_option:
                    result = api_client.query_transfer(start_station, end_station, travel_date.strftime('%m-%d'),
                                                       sort_param)
                else:
                    result = api_client.query_ticket(start_station, end_station, travel_date.strftime('%m-%d'),
                                                     sort_param)

                if result:
                    result_lines = result.strip().split("\n")
                    if result_lines[0] == "0":
                        st.write("No matching trains found.")
                    else:
                        num_trains = int(result_lines[0])
                        st.write(f"Number of matching trains: {num_trains}")

                        for line in result_lines[1:]:
                            st.write(line)
                else:
                    st.error("Failed to query tickets. Please check the input details.")


def app():
    st.title("Ticket Booking")

    option = st.selectbox("Select Functionality", ["Query Ticket", "Buy Ticket"])

    if option == "Query Ticket":
        query_ticket_form()
    elif option == "Buy Ticket":
        st.write("Buy Ticket functionality will be implemented soon.")


if __name__ == "__main__":
    app()
