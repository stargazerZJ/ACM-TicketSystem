import streamlit as st
from streamlit_server_state import server_state, server_state_lock
from utils.helpers import ensure_api_client, ensure_username

# Initialize global ApiClient instance
ensure_api_client()
ensure_username()

def system_management_page():
    st.session_state['management_page'] = 'System Management'
    st.title("System Management")

    st.write("Perform system management tasks below:")

    if st.button("Clean System"):
        st.session_state['confirm_clean'] = True

    if st.session_state.get('confirm_clean'):
        st.warning("Are you sure you want to clean the system? This action cannot be undone.")
        if st.button("Confirm Clean"):
            clean_system()
            st.session_state.pop('confirm_clean', None)
        if st.button("Cancel"):
            st.session_state.pop('confirm_clean', None)
            st.rerun()

def clean_system():
    with server_state_lock["api_client"]:
        api_client = server_state.api_client
        result = api_client.clean()

    if result == "0":
        st.success("System cleaned successfully.")
    else:
        st.error("System cleaning failed. Please try again.")

def app():
    st.title("System Management")

    if not st.session_state['username']:
        st.write("Welcome to the Train Ticket Booking System. Please log in to use the system.")
        return

    system_management_page()

if __name__ == "__main__":
    app()
