require "test_helper"

class ComputadorsControllerTest < ActionDispatch::IntegrationTest
  setup do
    @computador = computadors(:one)
  end

  test "should get index" do
    get computadors_url
    assert_response :success
  end

  test "should get new" do
    get new_computador_url
    assert_response :success
  end

  test "should create computador" do
    assert_difference("Computador.count") do
      post computadors_url, params: { computador: { marca: @computador.marca, numserie: @computador.numserie, pessoa_id: @computador.pessoa_id } }
    end

    assert_redirected_to computador_url(Computador.last)
  end

  test "should show computador" do
    get computador_url(@computador)
    assert_response :success
  end

  test "should get edit" do
    get edit_computador_url(@computador)
    assert_response :success
  end

  test "should update computador" do
    patch computador_url(@computador), params: { computador: { marca: @computador.marca, numserie: @computador.numserie, pessoa_id: @computador.pessoa_id } }
    assert_redirected_to computador_url(@computador)
  end

  test "should destroy computador" do
    assert_difference("Computador.count", -1) do
      delete computador_url(@computador)
    end

    assert_redirected_to computadors_url
  end
end
