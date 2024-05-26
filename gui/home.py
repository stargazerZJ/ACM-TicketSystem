import streamlit as st
from streamlit_server_state import server_state, server_state_lock
from utils.api_client import ApiClient
from utils.helpers import ensure_api_client

# Initialize global ApiClient instance
ensure_api_client()

# Initialize session state for user login
if 'username' not in st.session_state:
    st.session_state['username'] = None

def main_page():
    st.title("Train Ticket Booking System")
    if st.session_state['username']:
        st.write(f"Welcome, {st.session_state['username']}! Use the sidebar to navigate.")
    else:
        st.write("Welcome to the Train Ticket Booking System. Please log in to use the system.")

main_page()