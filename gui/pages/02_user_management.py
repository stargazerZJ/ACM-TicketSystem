import streamlit as st
from streamlit_server_state import server_state, server_state_lock
from utils.helpers import ensure_api_client

# Initialize global ApiClient instance
ensure_api_client()

def app():
    st.title("User Management")

    if 'username' in st.session_state and st.session_state['username']:
        st.write(f"Logged in as: {st.session_state['username']}")
        if st.button("Logout"):
            with server_state_lock["api_client"]:
                api_client = server_state.api_client
                if api_client.logout(st.session_state['username']):
                    st.session_state['username'] = None
                    st.success("Logged out successfully.")
                else:
                    st.error("Logout failed.")

        # Define the options for the selectbox
        options = ["Add User", "Query Profile", "Modify Profile"]
        choice = st.selectbox("Choose an action", options)

        if choice == "Add User":
            st.subheader("Add User")
            add_user_form()
        elif choice == "Query Profile":
            st.subheader("Query Profile")
            query_profile_form()
        elif choice == "Modify Profile":
            st.subheader("Modify Profile")
            modify_profile_form()

    else:
        login_form()
        add_user_form()

def login_form():
    st.subheader("Login")
    username = st.text_input("Username")
    password = st.text_input("Password", type="password")

    if st.button("Login"):
        with server_state_lock["api_client"]:
            api_client = server_state.api_client
            success = api_client.login(username, password) == '0'
            if success:
                st.session_state['username'] = username
                st.success("Logged in successfully.")
            else:
                st.error("Login failed. Please check your username and password.")

def add_user_form():
    with st.form("add_user_form"):
        username = st.text_input("New Username")
        password = st.text_input("New Password", type="password")
        name = st.text_input("Real Name")
        mailAddr = st.text_input("Email Address")
        privilege = st.number_input("Privilege", min_value=0, max_value=10, step=1)
        submitted = st.form_submit_button("Add User")
        if submitted:
            cur_username = st.session_state['username']
            with server_state_lock["api_client"]:
                api_client = server_state.api_client
                result = api_client.add_user(cur_username, username, password, name, mailAddr, privilege)
                if result == "0":
                    st.success("User added successfully.")
                else:
                    st.error("Failed to add user. The username might already exist.")

def query_profile_form():
    with st.form("query_profile_form"):
        query_username = st.text_input("Query Username")
        submitted = st.form_submit_button("Query Profile")
        if submitted:
            cur_username = st.session_state['username']
            with server_state_lock["api_client"]:
                api_client = server_state.api_client
                result = api_client.query_profile(cur_username, query_username)
                if result != "-1":
                    user_info = result.split()
                    st.write(f"Username: {user_info[0]}")
                    st.write(f"Name: {user_info[1]}")
                    st.write(f"Email: {user_info[2]}")
                    st.write(f"Privilege: {user_info[3]}")
                else:
                    st.error("Failed to query profile. Check the username or your permissions.")

def modify_profile_form():
    with st.form("modify_profile_form"):
        username = st.text_input("Username to Modify")
        password = st.text_input("New Password (optional)", type="password")
        name = st.text_input("New Real Name (optional)")
        mailAddr = st.text_input("New Email Address (optional)")
        privilege = st.number_input("New Privilege (optional)", min_value=0, max_value=10, step=1, value=10)
        submitted = st.form_submit_button("Modify Profile")
        if submitted:
            cur_username = st.session_state['username']
            with server_state_lock["api_client"]:
                api_client = server_state.api_client
                result = api_client.modify_profile(cur_username, username, password, name, mailAddr, privilege)
                if result != "-1":
                    user_info = result.split()
                    st.write(f"Username: {user_info[0]}")
                    st.write(f"Name: {user_info[1]}")
                    st.write(f"Email: {user_info[2]}")
                    st.write(f"Privilege: {user_info[3]}")
                    st.success("Profile modified successfully.")
                else:
                    st.error("Failed to modify profile. Check the username or your permissions.")

if __name__ == "__main__":
    app()
