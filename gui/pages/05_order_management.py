import datetime
import streamlit as st
from streamlit_server_state import server_state, server_state_lock
from utils.helpers import ensure_api_client, ensure_username

# Initialize global ApiClient instance
ensure_api_client()
ensure_username()


def query_order_form():
    st.session_state['order_page'] = 'Query Order'
    st.title("Query Orders")

    username = st.session_state.get('username', 'guest')

    with server_state_lock["api_client"]:
        api_client = server_state.api_client
        result = api_client.query_order(username)

    if result == "-1":
        st.write("No orders found.")
    else:
        result_lines = result.strip().split("\n")
        st.write(f"Number of orders: {result_lines[0]}")

        header_cols = st.columns(9)
        headers = ["Status", "Train ID", "From", "Leaving Time", "To", "Arriving Time", "Price", "Num Tickets",
                   "Action"]
        for header_col, header in zip(header_cols, headers):
            header_col.write(f"**{header}**")

        orders = []
        for line in result_lines[1:]:
            order_info = line.split()
            orders.append({
                "Status": order_info[0].strip("[]"),
                "Train ID": order_info[1],
                "From": order_info[2],
                "Leaving Time": f"{order_info[3]} {order_info[4]}",
                "To": order_info[6],
                "Arriving Time": f"{order_info[7]} {order_info[8]}",
                "Price": order_info[9],
                "Num Tickets": order_info[10]
            })

        for index, order in enumerate(orders):
            cols = st.columns(9)
            cols[0].write(order['Status'])
            cols[1].write(order['Train ID'])
            cols[2].write(order['From'])
            cols[3].write(order['Leaving Time'])
            cols[4].write(order['To'])
            cols[5].write(order['Arriving Time'])
            cols[6].write(order['Price'])
            cols[7].write(order['Num Tickets'])
            if order['Status'] != "refunded":
                if cols[8].button("Refund", key=f"refund_button_{index}"):
                    if st.session_state.get('confirm_refund') == index:
                        with server_state_lock["api_client"]:
                            api_client = server_state.api_client
                            result = api_client.refund_ticket(username, index + 1)
                        if result == "0":
                            st.success("Refund successful.")
                            st.session_state.pop('confirm_refund', None)
                        else:
                            st.error("Refund failed. Please try again.")
                    else:
                        st.session_state['confirm_refund'] = index

            if st.session_state.get('confirm_refund') == index:
                cols = st.columns(2)
                if cols[0].button("Confirm Refund", key=f"confirm_refund_button_{index}"):
                    with server_state_lock["api_client"]:
                        api_client = server_state.api_client
                        result = api_client.refund_ticket(username, index + 1)
                    if result == "0":
                        st.success("Refund successful.")
                        st.session_state.pop('confirm_refund', None)
                    else:
                        st.error("Refund failed. Please try again.")
                if cols[1].button("Cancel", key=f"cancel_refund_button_{index}"):
                    st.session_state.pop('confirm_refund', None)
                    st.rerun()


def app():
    st.title("Order Management")

    if not st.session_state['username']:
        st.write("Welcome to the Train Ticket Booking System. Please log in to use the system.")
        return

    query_order_form()


if __name__ == "__main__":
    app()
