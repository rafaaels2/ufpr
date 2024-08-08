json.extract! carro, :id, :crv, :nome, :marca, :pessoa_id, :created_at, :updated_at
json.url carro_url(carro, format: :json)
