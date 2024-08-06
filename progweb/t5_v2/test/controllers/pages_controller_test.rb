require "test_helper"

class PagesControllerTest < ActionDispatch::IntegrationTest
  test "should get home" do
    get pages_home_url
    assert_response :success
  end

  test "should get admin_view" do
    get pages_admin_view_url
    assert_response :success
  end

  test "should get user_view" do
    get pages_user_view_url
    assert_response :success
  end
end
