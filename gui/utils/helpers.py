from streamlit_server_state import server_state, server_state_lock
from utils.api_client import ApiClient

def ensure_api_client():
    with server_state_lock["api_client"]:
        if "api_client" not in server_state:
            server_state.api_client = ApiClient('./code')