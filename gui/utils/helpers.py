import streamlit as st
from streamlit_server_state import server_state, server_state_lock
from utils.api_client import ApiClient

def ensure_api_client():
    with server_state_lock["api_client"]:
        if "api_client" not in server_state:
            server_state.api_client = ApiClient()

def ensure_username():
    if 'username' not in st.session_state:
        st.session_state['username'] = None